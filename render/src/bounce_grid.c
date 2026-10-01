// The bounce grid update (ADR-0308 point 4): voe_render_bounce_update, and the
// two compute pipelines it dispatches, shaders/bounce.slang's reduce and gather,
// with their own set layout, pool and push block. device.c builds them after
// the frame objects and tears them down beside the bounce maps.
//
// ONE UPDATE: the target's grid's schedule lists the probes to refresh into this
// slot's probe list buffer, in the grid's own band; the first update after a
// bounce pass reduces the slot's bounce map to 64×64 VPLs in the slot's VPL
// buffer; gather refreshes the listed probes from them; the grid is marked
// updated in this frame slot, which is what pass.c reads. Barriers either side
// order it against the fragment reads of the grid and against the reduce.
//
// THE SET LAYOUT IS THIS FILE'S OWN, set 0 of a compute layout: 0 depth, 1 flux,
// 2 normal (sampled, depth in SHADER_READ_ONLY_OPTIMAL, the colour two in
// GENERAL, where bounce_map.c leaves them); 3 the VPLs, 4 the probe list; 5 the
// grid's three images as storage, GENERAL. One set per frame slot and grid,
// rewritten by every update: a slot's fence says its last use has finished,
// and one update a grid a frame is the most any set sees.
//
// THE PROBE LIST BUFFER is host-visible, coherent and mapped, per slot:
// (targets + 1) × VOE_RENDER_BOUNCE_PROBES³ indices, then as many blends, grid
// n's band at n × VOE_RENDER_BOUNCE_PROBES³ in each. The schedule writes into
// it directly; the submit makes the writes visible.
//
// TWO VIEWS IN ONE FRAME (bounce_scene.c proves it): each bounce pass followed
// by an update is reduced again, its VPLs about that pass's eye; the next
// pass's map writes wait on compute (bounce_map.c's barrier), the next reduce's
// VPL writes on the last gather's reads; each grid has its own set and band.
//
// CONSTRAINTS. The reduce runs once per bounce pass however many grids update;
// gather's naive loop is named in bounce.slang. The update records between
// passes, so it closes an open bounce pass and refuses inside any other.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static alignas(uint32_t) const unsigned char bounce_spv[] = {
#embed "bounce.spv"
};

#define REDUCE_ENTRY "reduce"
#define GATHER_ENTRY "gather"
#define BINDINGS 6
#define BLOCK 8
#define VPLS_SIDE (VOE_RENDER_BOUNCE_TEXELS / BLOCK)
#define VPL_BYTES 48
#define REDUCE_GROUP 8
#define GATHER_GROUP 64

// bounce.slang's push block, member for member at the same offsets.
struct bounce_push {
	float unproject[16];
	float corner[3];
	uint32_t probes;
	uint32_t cell[3];
	uint32_t count;
	uint32_t blends;
	float spacing;
	uint32_t reserved[2];
};

static_assert(offsetof(struct bounce_push, corner) == 64, "push corner");
static_assert(offsetof(struct bounce_push, cell) == 80, "push cell");
static_assert(offsetof(struct bounce_push, blends) == 96, "push blends");
static_assert(sizeof(struct bounce_push) == 112, "push size");

static uint32_t grid_count(const voe_render_device *device)
{
	return device->capacities.targets + 1;
}

static bool create_layouts(voe_render_device *device)
{
	VkDescriptorSetLayoutBinding bindings[BINDINGS];
	const VkDescriptorType types[BINDINGS] = {
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	};
	VkDescriptorSetLayoutCreateInfo set_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = BINDINGS,
		.pBindings = bindings,
	};
	const VkPushConstantRange push = {
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.size = sizeof(struct bounce_push),
	};
	VkPipelineLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push,
	};

	VOE_BASE_ASSERT(device != NULL, "bounce layouts on no device");
	VOE_BASE_ASSERT(device->bounce_set_layout == VK_NULL_HANDLE,
			"bounce layouts made twice");
	for (uint32_t i = 0; i < BINDINGS; i++)
		bindings[i] = (VkDescriptorSetLayoutBinding){
			.binding = i,
			.descriptorType = types[i],
			.descriptorCount = i == BINDINGS - 1 ? 3 : 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		};
	if (voe_render_vk.create_descriptor_set_layout(device->device, &set_info,
						       NULL,
						       &device->bounce_set_layout) !=
	    VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateDescriptorSetLayout failed for the bounce update");
		return false;
	}
	layout_info.pSetLayouts = &device->bounce_set_layout;
	if (voe_render_vk.create_pipeline_layout(device->device, &layout_info,
						 NULL, &device->bounce_layout) !=
	    VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreatePipelineLayout failed for the bounce update");
		return false;
	}
	return true;
}

static bool create_pipelines(voe_render_device *device)
{
	const VkShaderModuleCreateInfo module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = sizeof(bounce_spv),
		.pCode = (const uint32_t *)bounce_spv,
	};
	VkComputePipelineCreateInfo infos[2];
	const char *entries[2] = { REDUCE_ENTRY, GATHER_ENTRY };
	VkPipeline pipelines[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	VkShaderModule module;
	VkResult result;

	VOE_BASE_ASSERT(device != NULL, "bounce pipelines on no device");
	VOE_BASE_ASSERT(device->bounce_layout != VK_NULL_HANDLE,
			"bounce pipelines before their layout");
	if (voe_render_vk.create_shader_module(device->device, &module_info, NULL,
					       &module) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateShaderModule failed on bounce.spv");
		return false;
	}
	for (uint32_t i = 0; i < 2; i++)
		infos[i] = (VkComputePipelineCreateInfo){
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.module = module,
				.pName = entries[i],
			},
			.layout = device->bounce_layout,
		};
	result = voe_render_vk.create_compute_pipelines(device->device,
							VK_NULL_HANDLE, 2, infos,
							NULL, pipelines);
	voe_render_vk.destroy_shader_module(device->device, module, NULL);
	device->bounce_reduce = pipelines[0];
	device->bounce_gather = pipelines[1];
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateComputePipelines failed for the bounce update");
		return false;
	}
	return true;
}

// The pool and one set per frame slot and grid.
static bool create_sets(voe_render_device *device)
{
	const uint32_t sets = VOE_RENDER_FRAMES_IN_FLIGHT * grid_count(device);
	const VkDescriptorPoolSize sizes[3] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 3 * sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 * sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 * sets },
	};
	const VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = sets,
		.poolSizeCount = 3,
		.pPoolSizes = sizes,
	};
	VkDescriptorSetAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorSetCount = 1,
		.pSetLayouts = &device->bounce_set_layout,
	};

	VOE_BASE_ASSERT(device != NULL, "bounce sets on no device");
	VOE_BASE_ASSERT(device->bounce_set_layout != VK_NULL_HANDLE,
			"bounce sets before their layout");
	if (voe_render_vk.create_descriptor_pool(device->device, &pool_info, NULL,
						 &device->bounce_pool) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateDescriptorPool failed for the bounce update");
		return false;
	}
	allocate.descriptorPool = device->bounce_pool;
	for (uint32_t s = 0; s < VOE_RENDER_FRAMES_IN_FLIGHT; s++) {
		struct voe_render_frame *frame = &device->frames[s];

		frame->bounce_sets = calloc(grid_count(device),
					    sizeof(*frame->bounce_sets));
		if (frame->bounce_sets == NULL) {
			VOE_BASE_ERROR("render", "out of memory for the bounce sets");
			return false;
		}
		for (uint32_t g = 0; g < grid_count(device); g++)
			if (voe_render_vk.allocate_descriptor_sets(device->device,
								   &allocate,
								   &frame->bounce_sets[g]) !=
			    VK_SUCCESS) {
				VOE_BASE_ERROR("render", "vkAllocateDescriptorSets failed for the bounce update");
				return false;
			}
	}
	return true;
}

// Every slot's VPL buffer, device-local, and probe list buffer, mapped.
static bool create_buffers(voe_render_device *device)
{
	const VkDeviceSize lists = 2ull * grid_count(device) *
				   VOE_RENDER_BOUNCE_PROBES_ALL * sizeof(uint32_t);

	VOE_BASE_ASSERT(device != NULL, "bounce buffers on no device");
	VOE_BASE_ASSERT(sizeof(float) == sizeof(uint32_t),
			"a blend shares the list's word size");
	for (uint32_t s = 0; s < VOE_RENDER_FRAMES_IN_FLIGHT; s++) {
		struct voe_render_frame *frame = &device->frames[s];

		if (!voe_render_buffer_build(device, &frame->vpls,
					     (VkDeviceSize)VPLS_SIDE * VPLS_SIDE *
						     VPL_BYTES,
					     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
		    !voe_render_buffer_build(device, &frame->probe_lists, lists,
					     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;
		if (voe_render_vk.map_memory(device->device,
					     frame->probe_lists.memory, 0,
					     VK_WHOLE_SIZE, 0,
					     &frame->probe_lists_mapped) !=
		    VK_SUCCESS) {
			VOE_BASE_ERROR("render", "vkMapMemory failed on a bounce probe list");
			return false;
		}
	}
	return true;
}

bool voe_render_bounce_grid_startup(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "bounce update startup on no device");
	VOE_BASE_ASSERT(device->device != VK_NULL_HANDLE,
			"bounce update startup before the logical device");

	return create_layouts(device) && create_pipelines(device) &&
	       create_sets(device) && create_buffers(device);
}

void voe_render_bounce_grid_shutdown(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "bounce update shutdown on no device");
	VOE_BASE_ASSERT(device->capacities.targets < UINT32_MAX,
			"a grid count that wraps");

	for (uint32_t s = 0; s < VOE_RENDER_FRAMES_IN_FLIGHT; s++) {
		struct voe_render_frame *frame = &device->frames[s];

		voe_render_buffer_teardown(device, &frame->vpls);
		voe_render_buffer_teardown(device, &frame->probe_lists);
		frame->probe_lists_mapped = NULL;
		free(frame->bounce_sets);
		frame->bounce_sets = NULL;
	}
	if (device->device == VK_NULL_HANDLE)
		return;
	voe_render_vk.destroy_descriptor_pool(device->device, device->bounce_pool,
					      NULL);
	voe_render_vk.destroy_pipeline(device->device, device->bounce_reduce, NULL);
	voe_render_vk.destroy_pipeline(device->device, device->bounce_gather, NULL);
	voe_render_vk.destroy_pipeline_layout(device->device, device->bounce_layout,
					      NULL);
	voe_render_vk.destroy_descriptor_set_layout(device->device,
						    device->bounce_set_layout,
						    NULL);
	device->bounce_pool = VK_NULL_HANDLE;
	device->bounce_reduce = VK_NULL_HANDLE;
	device->bounce_gather = VK_NULL_HANDLE;
	device->bounce_layout = VK_NULL_HANDLE;
	device->bounce_set_layout = VK_NULL_HANDLE;
}

// Points `set` at `frame`'s map, VPLs and list, and `grid`'s images.
static void write_set(voe_render_device *device,
		      const struct voe_render_frame *frame,
		      const struct voe_render_bounce_grid *grid, VkDescriptorSet set)
{
	const VkDescriptorImageInfo map[3] = {
		{ .imageView = frame->bounce.depth.view,
		  .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ .imageView = frame->bounce.flux.view,
		  .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
		{ .imageView = frame->bounce.normal.view,
		  .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
	};
	const VkDescriptorBufferInfo buffers[2] = {
		{ frame->vpls.buffer, 0, VK_WHOLE_SIZE },
		{ frame->probe_lists.buffer, 0, VK_WHOLE_SIZE },
	};
	VkDescriptorImageInfo sh[3];
	VkWriteDescriptorSet writes[BINDINGS];

	VOE_BASE_ASSERT(set != VK_NULL_HANDLE, "writing no bounce set");
	VOE_BASE_ASSERT(grid->sh[0].view != VK_NULL_HANDLE,
			"writing a bounce set for a grid never built");
	for (uint32_t i = 0; i < 3; i++)
		sh[i] = (VkDescriptorImageInfo){
			.imageView = grid->sh[i].view,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		};
	for (uint32_t i = 0; i < BINDINGS; i++)
		writes[i] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = set,
			.dstBinding = i,
			.descriptorCount = 1,
		};
	for (uint32_t i = 0; i < 3; i++) {
		writes[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[i].pImageInfo = &map[i];
	}
	for (uint32_t i = 0; i < 2; i++) {
		writes[3 + i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[3 + i].pBufferInfo = &buffers[i];
	}
	writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writes[5].descriptorCount = 3;
	writes[5].pImageInfo = sh;
	voe_render_vk.update_descriptor_sets(device->device, BINDINGS, writes, 0,
					     NULL);
}

// One memory barrier between two compute or fragment uses.
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

	VOE_BASE_DEBUG_ASSERT(commands != VK_NULL_HANDLE, "a barrier on no recording");
	VOE_BASE_DEBUG_ASSERT(src != 0 && dst != 0, "a barrier with no stage");
	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
}

// The schedule into grid `index`'s band of the list; returns how many probes.
static uint32_t list_probes(voe_render_device *device,
			    const struct voe_render_frame *frame,
			    struct voe_render_bounce_grid *grid, uint32_t index,
			    const struct voe_render_bounce_update *update)
{
	uint32_t *words = frame->probe_lists_mapped;
	const size_t band = (size_t)index * VOE_RENDER_BOUNCE_PROBES_ALL;
	const size_t blends = (size_t)grid_count(device) *
			      VOE_RENDER_BOUNCE_PROBES_ALL;

	VOE_BASE_ASSERT(words != NULL, "a bounce probe list not mapped");
	VOE_BASE_ASSERT(index < grid_count(device), "a grid with no band");
	return voe_render_bounce_schedule_next(&grid->schedule, update->cell,
					       update->corner, &frame->bounce_sun,
					       update->stale, update->stale_count,
					       words + band,
					       (float *)(words + blends + band),
					       VOE_RENDER_BOUNCE_PROBES_ALL);
}

// The dispatches: reduce once per bounce pass, then gather over `count` probes.
static void record_update(voe_render_device *device,
			  struct voe_render_frame *frame,
			  const struct voe_render_bounce_grid *grid, uint32_t index,
			  const struct voe_render_bounce_update *update, uint32_t count)
{
	const VkPipelineStageFlags2 compute = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	const VkAccessFlags2 storage = VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
				       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	const voe_math_float4x4 unproject = voe_math_float4x4_inverse(
		voe_math_float4x4_mul(frame->bounce_view.projection,
				      frame->bounce_view.view));
	struct bounce_push push = {
		.corner = { update->corner.x, update->corner.y, update->corner.z },
		.probes = index * VOE_RENDER_BOUNCE_PROBES_ALL,
		.count = count,
		.blends = (grid_count(device) + index) * VOE_RENDER_BOUNCE_PROBES_ALL,
		.spacing = VOE_RENDER_BOUNCE_SPACING,
	};
	const VkDescriptorSet set = frame->bounce_sets[index];

	VOE_BASE_ASSERT(frame->bounced, "an update with no bounce pass");
	VOE_BASE_ASSERT(count <= VOE_RENDER_BOUNCE_PROBES_ALL, "a list past its band");
	memcpy(push.unproject, unproject.m, sizeof(push.unproject));
	for (uint32_t a = 0; a < 3; a++)
		push.cell[a] = voe_render_bounce_wrap(update->cell[a]);

	write_set(device, frame, grid, set);
	voe_render_vk.cmd_bind_descriptor_sets(frame->commands,
					       VK_PIPELINE_BIND_POINT_COMPUTE,
					       device->bounce_layout, 0, 1, &set,
					       0, NULL);
	voe_render_vk.cmd_push_constants(frame->commands, device->bounce_layout,
					 VK_SHADER_STAGE_COMPUTE_BIT, 0,
					 sizeof(push), &push);
	if (!frame->reduced) {
		record_barrier(frame->commands, compute, storage, compute, storage);
		voe_render_vk.cmd_bind_pipeline(frame->commands,
						VK_PIPELINE_BIND_POINT_COMPUTE,
						device->bounce_reduce);
		voe_render_vk.cmd_dispatch(frame->commands,
					   VPLS_SIDE / REDUCE_GROUP,
					   VPLS_SIDE / REDUCE_GROUP, 1);
		frame->reduced = true;
	}
	// The reduce's VPLs, and this grid's last reads and writes anywhere.
	record_barrier(frame->commands,
		       compute | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		       storage | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, compute,
		       storage);
	voe_render_vk.cmd_bind_pipeline(frame->commands,
					VK_PIPELINE_BIND_POINT_COMPUTE,
					device->bounce_gather);
	if (count > 0)
		voe_render_vk.cmd_dispatch(frame->commands,
					   (count + GATHER_GROUP - 1) / GATHER_GROUP,
					   1, 1);
	// The grid's new values, for the camera passes that sample it.
	record_barrier(frame->commands, compute, storage,
		       VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		       VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
}

// The grid `target` names, or NULL with a line when it names no live target.
static struct voe_render_bounce_grid *grid_of(voe_render_device *device,
					      voe_render_target target)
{
	struct voe_render_target_slot *own;

	VOE_BASE_ASSERT(device != NULL, "a grid of no device");
	if (target.index == VOE_RENDER_TARGET_WINDOW.index &&
	    target.generation == VOE_RENDER_TARGET_WINDOW.generation)
		return &device->window_grid;
	own = voe_render_target_at(device, target);
	if (own == NULL)
		VOE_BASE_ERROR("render", "a bounce update for a target id that names no live target");
	return own != NULL ? &own->grid : NULL;
}

bool voe_render_bounce_update(voe_render_device *device, voe_render_target target,
			      const struct voe_render_bounce_update *update)
{
	struct voe_render_frame *frame;
	struct voe_render_bounce_grid *grid;
	struct voe_render_bounce_mark *mark;
	uint32_t index;
	uint32_t count;

	VOE_BASE_ASSERT(device != NULL, "a bounce update on no device");
	VOE_BASE_ASSERT(update != NULL &&
				(update->stale_count == 0 || update->stale != NULL),
			"a bounce update with no update, or stale spheres with no array");
	if (!device->recording) {
		VOE_BASE_ERROR("render", "a bounce update with no frame open");
		return false;
	}
	if (device->pass_open && device->pass_bounce)
		voe_render_pass_end(device);
	if (device->pass_open) {
		VOE_BASE_ERROR("render", "a bounce update inside a pass — it records between passes");
		return false;
	}
	frame = voe_render_frame_open(device);
	if (!frame->bounced) {
		VOE_BASE_ERROR("render", "a bounce update with no bounce pass this frame");
		return false;
	}
	grid = grid_of(device, target);
	if (grid == NULL)
		return false;
	mark = &grid->updates[device->slot];
	if (mark->updated) {
		VOE_BASE_ERROR("render", "a second bounce update of one target in one frame");
		return false;
	}

	index = grid->descriptor / 3;
	count = list_probes(device, frame, grid, index, update);
	record_update(device, frame, grid, index, update, count);
	mark->updated = true;
	for (uint32_t a = 0; a < 3; a++)
		mark->cell[a] = update->cell[a];
	mark->corner[0] = update->corner.x;
	mark->corner[1] = update->corner.y;
	mark->corner[2] = update->corner.z;
	return true;
}
