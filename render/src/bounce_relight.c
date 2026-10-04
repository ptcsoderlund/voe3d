// The relight (ADR-0326 points 5 and 6): voe_render_bounce_relight, and the
// compute pipelines it dispatches, shaders/bounce_relight.slang's settle,
// relight and sum, with their own set layout, pool and push block. Prepare
// (pipeline.c) builds them as its last step and device.c tears them down beside
// the capture scratch, safely when they were never built; none without
// shaderOutputLayer. Until they are, the relight does nothing.
//
// ONE RELIGHT, after the begun target's capture passes: nothing on a volume not
// built, and nothing at all, not a barrier, when card 04 says no relight is
// needed. Otherwise, between barriers: the levels of a chain that lost its last
// light cleared, once; every probe marked changed (captured or emptied) listed
// into this slot's list and settled, one workgroup each; then for level k 1 to
// 3, for each chain n ≥ k holding a light (the sun at its bounces, each lamp at
// its own), relight over every probe, a barrier between levels; then sum; then
// the volume marked relit. The first barrier orders it after the capture copy,
// this frame's shadow passes and the last frame's reads; the last before this
// frame's fragment reads of the sum, validity and moments.
//
// THE SET LAYOUT IS THIS FILE'S OWN, set 0 of a compute layout: 0 the normal
// atlas, 1 the moments, 2 the validity, storage; 3 the list; 4 the seven grids'
// 21 images, storage; 5 the six levels' 18, 6 the validity and 7 the moments,
// sampled through the device's bounce and moments samplers; 8 the albedo atlas,
// sampled; 9 the slot's bounce shadow map (bounce_shadow.c's) and 10 its point
// shadow maps, through the shadow sampler; 11 its volume's region of the slot's
// record buffer. The volume's images rest in GENERAL,
// the maps where the shader reads them. One set per frame slot and volume (the
// window 0, target n n), rewritten by every relight: a slot's fence says its
// last use has finished, and a volume may have been rebuilt since.
//
// PER SLOT, host-visible, coherent and mapped, written directly, the submit
// making them visible: the list buffer, (targets + 1) ×
// VOE_RENDER_BOUNCE_PROBES_TOTAL words, volume n's band at n × the total, a
// word a toroidal probe index with bit 31 set when it holds a picture; and the
// record buffer, (targets + 1) regions device->relight_record_stride apart (the
// record rounded up to the card's uniform offset alignment), volume n's at n ×
// the stride, binding 11 of its set naming that region alone. One record per
// volume and not per slot (ADR-0330 point 2): the CPU writes it at the relight,
// so two volumes relit in one frame would both read the last one's. A struct
// voe_render_relight_record is the uniform the relight reads: the begun sun, the
// bounce shadow map's view × projection, its texel (the box's width over
// VOE_RENDER_BOUNCE_SHADOW_TEXELS) and whether this frame drew it, the sun's
// strength, the volume's placement (cell, corner and its begin's spacing) and the
// bouncing lamps.
//
// device->relight_dispatches counts every dispatch recorded, for a test to read.
//
// CONSTRAINTS. Listing tests every probe's changed bit, 6912 a relight; a scan
// by word would lift it if a profile names it. Every valid probe is relit at
// every level in use whatever changed (0326's fixed cost), up to six dispatches
// of 6912 workgroups.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static alignas(uint32_t) const unsigned char bounce_relight_spv[] = {
#embed "bounce_relight.spv"
};

#define BINDINGS 12
#define HOLDS 0x80000000u
// Seven grids of three images: six levels, then the sum.
#define LEVELS 6
#define GRID_IMAGES (3 * (LEVELS + 1))
#define LEVEL_IMAGES (3 * LEVELS)
#define GROUP 64

// bounce_relight.slang's push block, member for member: settle reads the first
// two, relight the rest.
struct relight_push {
	uint32_t first;
	uint32_t count;
	uint32_t chain;
	uint32_t level;
	uint32_t sun_chain;
	uint32_t lamps;
	uint32_t point_ready;
	uint32_t reserved;
};

static_assert(sizeof(struct relight_push) == 32, "relight push size");
static_assert(VOE_RENDER_BOUNCE_PROBES_TOTAL < HOLDS,
	      "a probe index leaves bit 31 for the holds flag");
static_assert(VOE_RENDER_BOUNCES_MAX == 3, "three chains of six levels");

static const VkDescriptorType BINDING_TYPES[BINDINGS] = {
	VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
};

static const uint32_t BINDING_COUNTS[BINDINGS] = {
	1, 1, 1, 1, GRID_IMAGES, LEVEL_IMAGES, 1, 1, 1, 1, 1, 1,
};

// Level (chain, level)'s grid, levels L(n, k) n-major from L(1, 1).
static uint32_t level_grid(uint32_t chain, uint32_t level)
{
	VOE_BASE_DEBUG_ASSERT(chain >= 1 && chain <= VOE_RENDER_BOUNCES_MAX &&
				      level >= 1 && level <= chain,
			      "a level no chain has");
	return chain * (chain - 1) / 2 + level - 1;
}

static uint32_t volume_count(const voe_render_device *device)
{
	return device->capacities.targets + 1;
}

static bool create_layouts(voe_render_device *device)
{
	VkDescriptorSetLayoutBinding bindings[BINDINGS];
	VkDescriptorSetLayoutCreateInfo set_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = BINDINGS,
		.pBindings = bindings,
	};
	const VkPushConstantRange push = {
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.size = sizeof(struct relight_push),
	};
	VkPipelineLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push,
	};

	for (uint32_t i = 0; i < BINDINGS; i++)
		bindings[i] = (VkDescriptorSetLayoutBinding){
			.binding = i,
			.descriptorType = BINDING_TYPES[i],
			.descriptorCount = BINDING_COUNTS[i],
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		};
	if (voe_render_vk.create_descriptor_set_layout(device->device, &set_info,
						       NULL,
						       &device->relight_set_layout) !=
	    VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateDescriptorSetLayout failed for the bounce relight");
		return false;
	}
	layout_info.pSetLayouts = &device->relight_set_layout;
	if (voe_render_vk.create_pipeline_layout(device->device, &layout_info,
						 NULL, &device->relight_layout) !=
	    VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreatePipelineLayout failed for the bounce relight");
		return false;
	}
	return true;
}

// settle, relight and sum, from the one module.
static bool create_pipelines(voe_render_device *device)
{
	static const char *const entries[3] = { "settle", "relight", "sum" };
	const VkShaderModuleCreateInfo module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = sizeof(bounce_relight_spv),
		.pCode = (const uint32_t *)bounce_relight_spv,
	};
	VkComputePipelineCreateInfo infos[3];
	VkPipeline pipelines[3] = { VK_NULL_HANDLE, VK_NULL_HANDLE,
				    VK_NULL_HANDLE };
	VkShaderModule module;
	VkResult result;

	if (voe_render_vk.create_shader_module(device->device, &module_info, NULL,
					       &module) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateShaderModule failed on bounce_relight.spv");
		return false;
	}
	for (uint32_t i = 0; i < 3; i++)
		infos[i] = (VkComputePipelineCreateInfo){
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.module = module,
				.pName = entries[i],
			},
			.layout = device->relight_layout,
		};
	result = voe_render_vk.create_compute_pipelines(device->device,
							VK_NULL_HANDLE, 3, infos,
							NULL, pipelines);
	voe_render_vk.destroy_shader_module(device->device, module, NULL);
	// A failed create leaves the failed ones VK_NULL_HANDLE; shutdown takes
	// back whichever were made.
	device->relight_settle = pipelines[0];
	device->relight_levels = pipelines[1];
	device->relight_sum = pipelines[2];
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateComputePipelines failed for the bounce relight");
		return false;
	}
	return true;
}

// One slot's mapped buffer of `size` bytes and `usage`.
static bool build_mapped(voe_render_device *device,
			 struct voe_render_buffer *buffer, VkDeviceSize size,
			 VkBufferUsageFlags usage, void **mapped)
{
	if (!voe_render_buffer_build(device, buffer, size, usage,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		return false;
	if (voe_render_vk.map_memory(device->device, buffer->memory, 0,
				     VK_WHOLE_SIZE, 0, mapped) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkMapMemory failed on a bounce relight buffer");
		return false;
	}
	return true;
}

// The record rounded up to the card's uniform offset alignment, a power of two,
// so each volume's region may be bound at its own offset.
static VkDeviceSize record_stride(const voe_render_device *device)
{
	VkPhysicalDeviceProperties properties;
	VkDeviceSize align;

	voe_render_vk.get_physical_device_properties(device->physical,
						     &properties);
	align = properties.limits.minUniformBufferOffsetAlignment;
	if (align == 0)
		align = 1;
	VOE_BASE_DEBUG_ASSERT((align & (align - 1)) == 0,
			      "a uniform offset alignment that is not a power of two");
	return (sizeof(struct voe_render_relight_record) + align - 1) & ~(align - 1);
}

// The pool, one set per frame slot and volume, and each slot's mapped list and
// record buffer, a region per volume.
static bool create_sets_and_buffers(voe_render_device *device)
{
	const uint32_t sets = VOE_RENDER_FRAMES_IN_FLIGHT * volume_count(device);
	const VkDescriptorPoolSize sizes[5] = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, (3 + GRID_IMAGES) * sets },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		  (LEVEL_IMAGES + 4) * sets },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sets },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sets },
	};
	const VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = sets,
		.poolSizeCount = 5,
		.pPoolSizes = sizes,
	};
	VkDescriptorSetAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorSetCount = 1,
		.pSetLayouts = &device->relight_set_layout,
	};
	const VkDeviceSize lists = (VkDeviceSize)volume_count(device) *
				   VOE_RENDER_BOUNCE_PROBES_TOTAL * sizeof(uint32_t);

	device->relight_record_stride = record_stride(device);
	if (voe_render_vk.create_descriptor_pool(device->device, &pool_info, NULL,
						 &device->relight_pool) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateDescriptorPool failed for the bounce relight");
		return false;
	}
	allocate.descriptorPool = device->relight_pool;
	for (uint32_t s = 0; s < VOE_RENDER_FRAMES_IN_FLIGHT; s++) {
		device->relight_sets[s] = calloc(volume_count(device),
						 sizeof(*device->relight_sets[s]));
		if (device->relight_sets[s] == NULL) {
			VOE_BASE_ERROR("render", "out of memory for the bounce relight sets");
			return false;
		}
		for (uint32_t v = 0; v < volume_count(device); v++)
			if (voe_render_vk.allocate_descriptor_sets(device->device,
								   &allocate,
								   &device->relight_sets[s][v]) !=
			    VK_SUCCESS) {
				VOE_BASE_ERROR("render", "vkAllocateDescriptorSets failed for the bounce relight");
				return false;
			}
		if (!build_mapped(device, &device->relight_lists[s], lists,
				  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				  &device->relight_mapped[s]) ||
		    !build_mapped(device, &device->relight_records[s],
				  volume_count(device) *
					  device->relight_record_stride,
				  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				  &device->relight_records_mapped[s]))
			return false;
	}
	return true;
}

bool voe_render_bounce_relight_startup(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL && device->device != VK_NULL_HANDLE,
			"bounce relight startup before the logical device");
	if (!device->output_layer)
		return true;
	device->relight_started = create_layouts(device) &&
				  create_pipelines(device) &&
				  create_sets_and_buffers(device);
	return device->relight_started;
}

void voe_render_bounce_relight_shutdown(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "bounce relight shutdown on no device");
	// Every handle below may be VK_NULL_HANDLE, which a vkDestroy takes.
	device->relight_started = false;
	for (uint32_t s = 0; s < VOE_RENDER_FRAMES_IN_FLIGHT; s++) {
		voe_render_buffer_teardown(device, &device->relight_lists[s]);
		voe_render_buffer_teardown(device, &device->relight_records[s]);
		device->relight_mapped[s] = NULL;
		device->relight_records_mapped[s] = NULL;
		free(device->relight_sets[s]);
		device->relight_sets[s] = NULL;
	}
	if (device->device == VK_NULL_HANDLE)
		return;
	voe_render_vk.destroy_descriptor_pool(device->device, device->relight_pool,
					      NULL);
	voe_render_vk.destroy_pipeline(device->device, device->relight_settle, NULL);
	voe_render_vk.destroy_pipeline(device->device, device->relight_levels, NULL);
	voe_render_vk.destroy_pipeline(device->device, device->relight_sum, NULL);
	voe_render_vk.destroy_pipeline_layout(device->device, device->relight_layout,
					      NULL);
	voe_render_vk.destroy_descriptor_set_layout(device->device,
						    device->relight_set_layout,
						    NULL);
	device->relight_pool = VK_NULL_HANDLE;
	device->relight_settle = VK_NULL_HANDLE;
	device->relight_levels = VK_NULL_HANDLE;
	device->relight_sum = VK_NULL_HANDLE;
	device->relight_layout = VK_NULL_HANDLE;
	device->relight_set_layout = VK_NULL_HANDLE;
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
	VOE_BASE_DEBUG_ASSERT((chains & ~0xeu) == 0, "a chain past three");
	return chains;
}

// Every changed probe of `p` into `words`, flagged when it holds a picture;
// returns how many.
static uint32_t list_changed(const voe_render_bounce_probes *p, uint32_t *words)
{
	uint32_t count = 0;

	for (uint32_t probe = 0; probe < VOE_RENDER_BOUNCE_PROBES_TOTAL; probe++) {
		const uint32_t bit = 1u << (probe % 32);

		if ((p->changed[probe / 32] & bit) != 0)
			words[count++] = probe |
					 ((p->holds[probe / 32] & bit) != 0 ? HOLDS : 0u);
	}
	return count;
}

// Volume `index`'s region of this slot's record buffer from the begin: `lights`,
// the slot's bounce shadow map and the volume's placement. The map's texel is the
// box's width over its texels: the orthographic projection's x scale is 2 /
// width, and row 0 of view × projection is that scale times a unit row of the
// view's rotation.
static void write_record(voe_render_device *device, uint32_t index,
			 const voe_render_bounce_lights *lights)
{
	const struct voe_render_bounce_frame *begun = &device->bounce_frame;
	const struct voe_render_bounce_shadow *map =
		&device->frames[device->slot].bounce_shadow;
	const float *row = map->light.m[0];
	const float scale = sqrtf(row[0] * row[0] + row[1] * row[1] +
				  row[2] * row[2]);
	const uint32_t size[3] = { VOE_RENDER_BOUNCE_PROBES_XZ,
				   VOE_RENDER_BOUNCE_PROBES_Y,
				   VOE_RENDER_BOUNCE_PROBES_XZ };
	struct voe_render_relight_record record = {
		.sun = lights->sun,
		.sun_map = map->light,
		.sun_drawn = map->drawn ? 1u : 0u,
		.sun_texel = map->drawn && scale > 0.0f ?
				     2.0f / (scale * VOE_RENDER_BOUNCE_SHADOW_TEXELS) :
				     0.0f,
		.corner = { begun->corner.x, begun->corner.y, begun->corner.z },
		.sun_strength = lights->sun_strength,
		.spacing = begun->spacing,
	};

	VOE_BASE_DEBUG_ASSERT(device->relight_records_mapped[device->slot] != NULL,
			      "a relight record never mapped");
	VOE_BASE_DEBUG_ASSERT(index < volume_count(device), "a volume with no region");
	for (uint32_t a = 0; a < 3; a++)
		record.cell[a] = voe_render_bounce_probe_wrap(begun->cell[a], size[a]);
	memcpy(record.lamps, lights->lamps, sizeof(record.lamps));
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
	VkDescriptorImageInfo storage[3 + GRID_IMAGES];
	VkDescriptorImageInfo levels[LEVEL_IMAGES];
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
	const VkDescriptorImageInfo *images[BINDINGS] = {
		&storage[0], &storage[1], &storage[2], NULL,
		&storage[3], levels,	  &sampled[0], &sampled[1],
		&sampled[2], &sampled[3], &sampled[4], NULL,
	};
	VkWriteDescriptorSet writes[BINDINGS];

	storage[0] = (VkDescriptorImageInfo){ .imageView = volume->normal.view,
					      .imageLayout = VK_IMAGE_LAYOUT_GENERAL };
	storage[1] = storage[0];
	storage[1].imageView = volume->moments.view;
	storage[2] = storage[0];
	storage[2].imageView = volume->validity.view;
	for (uint32_t i = 0; i < GRID_IMAGES; i++) {
		storage[3 + i] = storage[0];
		storage[3 + i].imageView = volume->irradiance[i / 3][i % 3].view;
	}
	for (uint32_t i = 0; i < LEVEL_IMAGES; i++)
		levels[i] = (VkDescriptorImageInfo){
			device->bounce_sampler, volume->irradiance[i / 3][i % 3].view,
			VK_IMAGE_LAYOUT_GENERAL
		};
	for (uint32_t i = 0; i < BINDINGS; i++)
		writes[i] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = set,
			.dstBinding = i,
			.descriptorCount = BINDING_COUNTS[i],
			.descriptorType = BINDING_TYPES[i],
			.pImageInfo = images[i],
			.pBufferInfo = i == 3 ? &buffers[0] :
				       i == 11 ? &buffers[1] : NULL,
		};
	voe_render_vk.update_descriptor_sets(device->device, BINDINGS, writes, 0,
					     NULL);
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
			    const struct relight_push *push, uint32_t groups)
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
			  VkDescriptorSet set, struct relight_push push,
			  uint32_t held)
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

// The whole relight of volume `index`, `count` probes listed in its band.
static void record_relight(voe_render_device *device,
			   struct voe_render_bounce_volume *volume, uint32_t index,
			   uint32_t count, const voe_render_bounce_lights *lights)
{
	const VkPipelineStageFlags2 compute = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	const VkPipelineStageFlags2 clear = VK_PIPELINE_STAGE_2_CLEAR_BIT;
	const VkAccessFlags2 storage = VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
				       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	const VkAccessFlags2 sampled = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
	const VkCommandBuffer commands = voe_render_frame_open(device)->commands;
	const VkDescriptorSet set = device->relight_sets[device->slot][index];
	const uint32_t held = chains_held(lights);
	const struct relight_push push = {
		.first = index * VOE_RENDER_BOUNCE_PROBES_TOTAL,
		.count = count,
		.sun_chain = lights->sun_bounces,
		.lamps = lights->lamp_count,
		.point_ready = voe_render_point_shadows_ready(device) ? 1u : 0u,
	};

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
	volume = voe_render_bounce_volume_of(device, device->bounce_target);
	if (!volume->built)
		return;
	voe_render_bounce_begun_lights(device, &lights);
	if (!voe_render_bounce_probes_relight_needed(&volume->probes, &lights))
		return;

	// The window's volume is 0 and target n's n, its id's index.
	index = device->bounce_target.index;
	VOE_BASE_DEBUG_ASSERT(index < volume_count(device), "a volume with no band");
	count = list_changed(&volume->probes,
			     (uint32_t *)device->relight_mapped[device->slot] +
				     (size_t)index * VOE_RENDER_BOUNCE_PROBES_TOTAL);
	record_relight(device, volume, index, count, &lights);
	voe_render_bounce_probes_relit(&volume->probes, &lights);
}
