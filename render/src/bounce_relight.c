// The relight (ADR-0326 points 5 and 6): voe_render_bounce_relight, which
// records shaders/bounce_relight.slang's settle, relight and sum over a begun
// volume. What it runs on, the pipelines, set layout, pool, push block and each
// slot's list and record buffers, is built by bounce_relight_build.c, whose
// header says what each holds; until it is, the relight does nothing.
//
// ONE RELIGHT, after the begun target's capture passes: nothing on a volume not
// built, and nothing at all, not a barrier, when card 04 says no relight is
// needed. Otherwise, between barriers: the levels of a chain that lost its last
// light cleared, once; listed into this slot's list in two runs (ADR-0389
// point 4), every probe marked changed (captured or emptied), then every one
// fading in that did not change, each word with its readiness (the layout in
// bounce_relight.h), and every listed probe settled, one workgroup each; then for level k 1 to
// 3, for each chain n ≥ k holding a light (each sun and each lamp at its own
// bounces), relight over every probe, a barrier between levels; then sum; then
// the volume marked relit, all of it timed as the pass `bounce relight`
// (pass_timing.c). The first barrier orders it after the capture copy,
// this frame's shadow passes and the last frame's reads; the last before this
// frame's fragment reads of the sum, validity and moments. Each relight
// rewrites its volume's set for this slot and its region of the record buffer.
//
// A SETTLE-ONLY BEGIN: with no probe changed and the lights as last relit, only
// fading probes listed, the first barrier, the settle writing their readiness
// and the barrier before the frame's reads, and nothing else: no clear, level
// or sum. The volume is still marked relit.
//
// device->relight_dispatches counts every dispatch recorded, for a test to read.
//
// CONSTRAINTS. Listing scans every probe twice, 2 × 6912 a relight; a scan
// by word would lift it if a profile names it. Every valid probe is relit at
// every level in use whatever changed (0326's fixed cost), up to six dispatches
// of 6912 workgroups.
#include "bounce_relight.h"

#include <base/assert.h>

#include <math.h>
#include <stddef.h>
#include <string.h>

#define GROUP 64

// Level (chain, level)'s grid, levels L(n, k) n-major from L(1, 1).
static uint32_t level_grid(uint32_t chain, uint32_t level)
{
	VOE_BASE_DEBUG_ASSERT(chain >= 1 && chain <= VOE_RENDER_BOUNCES_MAX &&
				      level >= 1 && level <= chain,
			      "a level no chain has");
	return chain * (chain - 1) / 2 + level - 1;
}

// The bouncing lights this frame's begin placed, as card 04 compares them.
void voe_render_bounce_begun_lights(const voe_render_device *device,
				    voe_render_bounce_lights *lights)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL && lights != NULL,
			      "the begun lights of no device or into nowhere");
	memset(lights, 0, sizeof(*lights));
	lights->sun = device->bounce_frame.sun;
	lights->sun_bounces = device->bounce_frame.sun_bounces;
	lights->sun_strength = device->bounce_frame.sun_strength;
	lights->lamp_count = device->bounce_frame.points.count;
	memcpy(lights->lamps, device->bounce_lamps,
	       lights->lamp_count * sizeof(lights->lamps[0]));
	lights->blocker_count = device->bounce_frame.blockers.count;
	memcpy(lights->blockers, device->bounce_blockers,
	       lights->blocker_count * sizeof(lights->blockers[0]));
	lights->walls = device->bounce_frame.blockers.walls;
	lights->indoors = device->bounce_frame.blockers.indoors;
	lights->sun_mask = device->bounce_frame.blockers.sun;
	lights->more_count = device->bounce_frame.more.count;
	for (uint32_t i = 0; i < lights->more_count; i++)
		lights->more[i] = (voe_render_bounce_sun){
			.light = device->bounce_suns[i].light,
			.bounces = device->bounce_suns[i].bounces,
			.strength = device->bounce_suns[i].bounce_strength,
			.mask = device->bounce_suns[i].blockers,
		};
}

// The chains `lights` hold a light in, bit n for chain n.
static uint32_t chains_held(const voe_render_bounce_lights *lights)
{
	uint32_t chains = lights->sun_bounces > 0 ? 1u << lights->sun_bounces : 0u;

	VOE_BASE_DEBUG_ASSERT(lights->lamp_count <= VOE_RENDER_BOUNCE_LAMPS,
			      "more bouncing lamps than the record holds");
	for (uint32_t i = 0; i < lights->lamp_count; i++)
		if (lights->lamps[i].bounces > 0)
			chains |= 1u << lights->lamps[i].bounces;
	// A further sun with no bounces or no intensity holds no chain.
	for (uint32_t i = 0; i < lights->more_count; i++)
		if (lights->more[i].bounces > 0 &&
		    lights->more[i].light.intensity > 0.0f)
			chains |= 1u << lights->more[i].bounces;
	VOE_BASE_DEBUG_ASSERT((chains & ~0xeu) == 0, "a chain past three");
	return chains;
}

// Probe `probe` of `p` as a list word (bounce_relight.h).
static uint32_t list_word(const voe_render_bounce_probes *p, uint32_t probe)
{
	const uint32_t bit = 1u << (probe % 32);

	VOE_BASE_DEBUG_ASSERT(probe <= VOE_RENDER_RELIGHT_PROBE &&
				      p->ready[probe] <= VOE_RENDER_RELIGHT_READY,
			      "a probe or readiness past its list word field");
	return probe |
	       ((uint32_t)p->ready[probe] << VOE_RENDER_RELIGHT_READY_SHIFT) |
	       ((p->holds[probe / 32] & bit) != 0 ? VOE_RENDER_RELIGHT_HOLDS : 0u);
}

// Every changed probe of `p` into `words`, then every fading one that did not
// change; `changed` says how many of the first run. Returns how many in all.
static uint32_t list_probes(const voe_render_bounce_probes *p, uint32_t *words,
			    uint32_t *changed)
{
	uint32_t count = 0;

	VOE_BASE_DEBUG_ASSERT(p != NULL && words != NULL && changed != NULL,
			      "listing no probes or into nowhere");
	for (uint32_t probe = 0; probe < VOE_RENDER_BOUNCE_PROBES_TOTAL; probe++)
		if ((p->changed[probe / 32] & (1u << (probe % 32))) != 0)
			words[count++] = list_word(p, probe);
	*changed = count;
	for (uint32_t probe = 0; probe < VOE_RENDER_BOUNCE_PROBES_TOTAL; probe++) {
		const uint32_t bit = 1u << (probe % 32);

		if ((p->fading[probe / 32] & bit) != 0 &&
		    (p->changed[probe / 32] & bit) == 0)
			words[count++] = list_word(p, probe);
	}
	VOE_BASE_DEBUG_ASSERT(count <= VOE_RENDER_BOUNCE_PROBES_TOTAL,
			      "more probes listed than a grid holds");
	return count;
}

// Layer `layer`'s texel of `map`, nought when the begin drew none: the box's
// width over its texels. The orthographic projection's x scale is 2 / width,
// and row 0 of view × projection is that scale times a unit row of the view's
// rotation.
static float map_texel(const struct voe_render_bounce_shadow *map,
		       uint32_t layer)
{
	const float *row;
	float scale;

	VOE_BASE_DEBUG_ASSERT(layer < VOE_RENDER_DIRECTIONAL_LIGHTS,
			      "a sun map layer past the map");
	row = map->light[layer].m[0];
	scale = sqrtf(row[0] * row[0] + row[1] * row[1] + row[2] * row[2]);
	return map->drawn[layer] && scale > 0.0f ?
		       2.0f / (scale * VOE_RENDER_BOUNCE_SHADOW_TEXELS) :
		       0.0f;
}

// Volume `index`'s region of this slot's record buffer from the begin: `lights`,
// the slot's bounce shadow map and the volume's placement; further sun i reads
// layer i + 1, as the begin gave it.
static void write_record(voe_render_device *device, uint32_t index,
			 const voe_render_bounce_lights *lights)
{
	const struct voe_render_bounce_frame *begun = &device->bounce_frame;
	const struct voe_render_bounce_shadow *map =
		&device->frames[device->slot].bounce_shadow;
	const uint32_t size[3] = { VOE_RENDER_BOUNCE_PROBES_XZ,
				   VOE_RENDER_BOUNCE_PROBES_Y,
				   VOE_RENDER_BOUNCE_PROBES_XZ };
	struct voe_render_relight_record record = {
		.sun = lights->sun,
		.sun_map = map->light[0],
		.sun_drawn = map->drawn[0] ? 1u : 0u,
		.sun_texel = map_texel(map, 0),
		.corner = { begun->corner.x, begun->corner.y, begun->corner.z },
		.sun_strength = lights->sun_strength,
		.spacing = begun->spacing,
		.blocker_count = lights->blocker_count,
		.walls = lights->walls,
		.indoors = lights->indoors,
		.sun_mask = lights->sun_mask,
		.more_count = lights->more_count,
	};

	VOE_BASE_DEBUG_ASSERT(device->relight_records_mapped[device->slot] != NULL,
			      "a relight record never mapped");
	VOE_BASE_DEBUG_ASSERT(index < voe_render_relight_volume_count(device),
			      "a volume with no region");
	VOE_BASE_DEBUG_ASSERT(lights->more_count < VOE_RENDER_DIRECTIONAL_LIGHTS,
			      "more further suns than the record holds");
	for (uint32_t i = 0; i < lights->more_count; i++)
		record.more[i] = (struct voe_render_relight_sun){
			.light = lights->more[i].light,
			.map = map->light[i + 1],
			.texel = map_texel(map, i + 1),
			.drawn = map->drawn[i + 1] ? 1u : 0u,
			.strength = lights->more[i].strength,
			.mask = lights->more[i].mask,
			.bounces = lights->more[i].bounces,
		};
	for (uint32_t a = 0; a < 3; a++)
		record.cell[a] = voe_render_bounce_probe_wrap(begun->cell[a], size[a]);
	memcpy(record.lamps, lights->lamps, sizeof(record.lamps));
	memcpy(record.blockers, lights->blockers, sizeof(record.blockers));
	memcpy((unsigned char *)device->relight_records_mapped[device->slot] +
		       index * device->relight_record_stride,
	       &record, sizeof(record));
}

// Points `set` at `volume`'s images, slot `slot`'s maps and list, and volume
// `index`'s region of its record buffer.
static void write_set(voe_render_device *device,
		      const struct voe_render_bounce_volume *volume,
		      uint32_t index, uint32_t slot, VkDescriptorSet set)
{
	const struct voe_render_frame *frame = &device->frames[slot];
	VkDescriptorImageInfo storage[3 + VOE_RENDER_RELIGHT_GRID_IMAGES];
	VkDescriptorImageInfo levels[VOE_RENDER_RELIGHT_LEVEL_IMAGES];
	const VkDescriptorImageInfo sampled[5] = {
		{ device->bounce_sampler, volume->validity.view,
		  VK_IMAGE_LAYOUT_GENERAL },
		{ device->moments_sampler, volume->moments.view,
		  VK_IMAGE_LAYOUT_GENERAL },
		{ VK_NULL_HANDLE, volume->albedo.view, VK_IMAGE_LAYOUT_GENERAL },
		{ device->shadow_sampler, frame->bounce_shadow.map.view,
		  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ device->shadow_sampler, frame->point_shadow.sampled,
		  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
	};
	const VkDescriptorBufferInfo buffers[2] = {
		{ device->relight_lists[slot].buffer, 0, VK_WHOLE_SIZE },
		{ device->relight_records[slot].buffer,
		  index * device->relight_record_stride,
		  sizeof(struct voe_render_relight_record) },
	};
	const VkDescriptorImageInfo *images[VOE_RENDER_RELIGHT_BINDINGS] = {
		&storage[0], &storage[1], &storage[2], NULL,
		&storage[3], levels,	  &sampled[0], &sampled[1],
		&sampled[2], &sampled[3], &sampled[4], NULL,
	};
	VkWriteDescriptorSet writes[VOE_RENDER_RELIGHT_BINDINGS];

	storage[0] = (VkDescriptorImageInfo){ .imageView = volume->normal.view,
					      .imageLayout = VK_IMAGE_LAYOUT_GENERAL };
	storage[1] = storage[0];
	storage[1].imageView = volume->moments.view;
	storage[2] = storage[0];
	storage[2].imageView = volume->validity.view;
	for (uint32_t i = 0; i < VOE_RENDER_RELIGHT_GRID_IMAGES; i++) {
		storage[3 + i] = storage[0];
		storage[3 + i].imageView = volume->irradiance[i / 3][i % 3].view;
	}
	for (uint32_t i = 0; i < VOE_RENDER_RELIGHT_LEVEL_IMAGES; i++)
		levels[i] = (VkDescriptorImageInfo){
			device->bounce_sampler, volume->irradiance[i / 3][i % 3].view,
			VK_IMAGE_LAYOUT_GENERAL
		};
	for (uint32_t i = 0; i < VOE_RENDER_RELIGHT_BINDINGS; i++)
		writes[i] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = set,
			.dstBinding = i,
			.descriptorCount = voe_render_relight_binding_counts[i],
			.descriptorType = voe_render_relight_binding_types[i],
			.pImageInfo = images[i],
			.pBufferInfo = i == 3 ? &buffers[0] :
				       i == 11 ? &buffers[1] : NULL,
		};
	voe_render_vk.update_descriptor_sets(device->device,
					     VOE_RENDER_RELIGHT_BINDINGS, writes,
					     0, NULL);
}

// One memory barrier between `src` and `dst` uses.
static void record_barrier(VkCommandBuffer commands, VkPipelineStageFlags2 src,
			   VkAccessFlags2 src_access, VkPipelineStageFlags2 dst,
			   VkAccessFlags2 dst_access)
{
	const VkMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = src,
		.srcAccessMask = src_access,
		.dstStageMask = dst,
		.dstAccessMask = dst_access,
	};
	const VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &barrier,
	};

	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
}

// `pipeline` over `groups` workgroups with `push`, counted.
static void record_dispatch(voe_render_device *device, VkCommandBuffer commands,
			    VkPipeline pipeline, VkDescriptorSet set,
			    const struct voe_render_relight_push *push,
			    uint32_t groups)
{
	VOE_BASE_DEBUG_ASSERT(pipeline != VK_NULL_HANDLE && groups > 0,
			      "dispatching no relight pipeline or nothing");
	voe_render_vk.cmd_bind_pipeline(commands, VK_PIPELINE_BIND_POINT_COMPUTE,
					pipeline);
	voe_render_vk.cmd_bind_descriptor_sets(commands,
					       VK_PIPELINE_BIND_POINT_COMPUTE,
					       device->relight_layout, 0, 1, &set,
					       0, NULL);
	voe_render_vk.cmd_push_constants(commands, device->relight_layout,
					 VK_SHADER_STAGE_COMPUTE_BIT, 0,
					 sizeof(*push), push);
	voe_render_vk.cmd_dispatch(commands, groups, 1, 1);
	device->relight_dispatches++;
}

// Chain `chain`'s levels of `volume` cleared to nought, in GENERAL.
static void record_clear_chain(VkCommandBuffer commands,
			       const struct voe_render_bounce_volume *volume,
			       uint32_t chain)
{
	const VkClearColorValue nought = { .float32 = { 0.0f } };
	const VkImageSubresourceRange range = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.levelCount = 1,
		.layerCount = 1,
	};

	for (uint32_t level = 1; level <= chain; level++)
		for (uint32_t c = 0; c < 3; c++)
			voe_render_vk.cmd_clear_color_image(
				commands,
				volume->irradiance[level_grid(chain, level)][c].image,
				VK_IMAGE_LAYOUT_GENERAL, &nought, 1, &range);
}

// Every level in use, a barrier after each, then the sum and the barrier
// before the frame's reads.
static void record_levels(voe_render_device *device, VkCommandBuffer commands,
			  VkDescriptorSet set,
			  struct voe_render_relight_push push, uint32_t held)
{
	const VkPipelineStageFlags2 compute = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	const VkAccessFlags2 reads = VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
				     VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;

	for (uint32_t level = 1; level <= VOE_RENDER_BOUNCES_MAX; level++) {
		bool dispatched = false;

		for (uint32_t chain = level; chain <= VOE_RENDER_BOUNCES_MAX; chain++) {
			if ((held & (1u << chain)) == 0)
				continue;
			push.chain = chain;
			push.level = level;
			record_dispatch(device, commands, device->relight_levels,
					set, &push, VOE_RENDER_BOUNCE_PROBES_TOTAL);
			dispatched = true;
		}
		if (dispatched)
			record_barrier(commands, compute,
				       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, compute,
				       reads);
	}
	// One thread a texel: two a probe along x (ADR-0327).
	record_dispatch(device, commands, device->relight_sum, set, &push,
			(2 * VOE_RENDER_BOUNCE_PROBES_TOTAL + GROUP - 1) / GROUP);
	record_barrier(commands, compute, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
		       compute | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, reads);
}

// The whole relight of volume `index`, `count` probes listed in its band, the
// first `changed` of them changed; with `relights` false (only fading probes,
// the lights as relit) the settle and its barriers alone.
static void record_relight(voe_render_device *device,
			   struct voe_render_bounce_volume *volume, uint32_t index,
			   uint32_t count, uint32_t changed, bool relights,
			   const voe_render_bounce_lights *lights)
{
	const VkPipelineStageFlags2 compute = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	const VkPipelineStageFlags2 clear = VK_PIPELINE_STAGE_2_CLEAR_BIT;
	const VkAccessFlags2 storage = VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
				       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	const VkAccessFlags2 sampled = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
	const VkCommandBuffer commands = voe_render_frame_open(device)->commands;
	const VkDescriptorSet set = device->relight_sets[device->slot][index];
	const uint32_t held = chains_held(lights);
	const struct voe_render_relight_push push = {
		.first = index * VOE_RENDER_BOUNCE_PROBES_TOTAL,
		.count = count,
		.sun_chain = lights->sun_bounces,
		.lamps = lights->lamp_count,
		.point_ready = voe_render_point_shadows_ready(device) ? 1u : 0u,
		.changed = changed,
	};

	VOE_BASE_DEBUG_ASSERT(changed <= count && (relights || count > 0),
			      "more changed than listed, or a settle of nothing");
	write_set(device, volume, index, device->slot, set);
	write_record(device, index, lights);
	// The capture copy, this frame's shadow maps, and every earlier read or
	// write of the volume's images.
	record_barrier(commands,
		       VK_PIPELINE_STAGE_2_COPY_BIT | clear | compute |
			       VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
			       VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
			       VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
		       VK_ACCESS_2_TRANSFER_WRITE_BIT | storage | sampled |
			       VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		       compute | clear,
		       storage | sampled | VK_ACCESS_2_TRANSFER_WRITE_BIT);
	if (!relights) {
		record_dispatch(device, commands, device->relight_settle, set,
				&push, count);
		// The new readiness, before this frame's reads.
		record_barrier(commands, compute,
			       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			       compute | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			       storage | sampled);
		return;
	}
	for (uint32_t chain = 1; chain <= VOE_RENDER_BOUNCES_MAX; chain++)
		if ((volume->chains_lit & ~held & (1u << chain)) != 0)
			record_clear_chain(commands, volume, chain);
	volume->chains_lit = held;
	if (count > 0)
		record_dispatch(device, commands, device->relight_settle, set,
				&push, count);
	// The cleared levels and the new validity and moments, for the relight.
	record_barrier(commands, compute | clear,
		       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT |
			       VK_ACCESS_2_TRANSFER_WRITE_BIT,
		       compute, storage | sampled);
	record_levels(device, commands, set, push, held);
}

void voe_render_bounce_relight(voe_render_device *device)
{
	struct voe_render_bounce_volume *volume;
	voe_render_bounce_lights lights;
	uint32_t index;
	uint32_t count;
	uint32_t changed;
	bool relights;

	VOE_BASE_ASSERT(device != NULL, "a bounce relight on no device");
	VOE_BASE_ASSERT(device->recording, "a bounce relight with no frame open");
	VOE_BASE_ASSERT(!device->pass_open,
			"a bounce relight inside a pass — it records between passes");
	if (!device->relight_started)
		return;
	// Without shaderOutputLayer a begin records nothing to assert on.
	VOE_BASE_ASSERT(device->bounce_begun || !device->output_layer,
			"a bounce relight with no voe_render_bounce_begin this frame");
	if (!device->bounce_begun)
		return;
	volume = voe_render_bounce_volume_of(device, device->bounce_target,
					     device->bounce_volume);
	if (!volume->built)
		return;
	voe_render_bounce_begun_lights(device, &lights);
	if (!voe_render_bounce_probes_relight_needed(&volume->probes, &lights))
		return;

	// The window's slot is 0 and target n's n, its id's index.
	index = voe_render_bounce_volume_index(device->bounce_target.index,
					       device->bounce_volume);
	VOE_BASE_DEBUG_ASSERT(index < voe_render_relight_volume_count(device),
			      "a volume with no band");
	count = list_probes(&volume->probes,
			    (uint32_t *)device->relight_mapped[device->slot] +
				    (size_t)index * VOE_RENDER_BOUNCE_PROBES_TOTAL,
			    &changed);
	relights = changed > 0 ||
		   voe_render_bounce_probes_lights_changed(&volume->probes,
							   &lights);
	voe_render_pass_timing_open(device, voe_render_frame_open(device),
				    "bounce relight");
	record_relight(device, volume, index, count, changed, relights, &lights);
	voe_render_pass_timing_close(device, voe_render_frame_open(device));
	voe_render_bounce_probes_relit(&volume->probes, &lights);
}
