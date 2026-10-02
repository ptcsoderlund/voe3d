// The relight (ADR-0326 points 5 and 6): voe_render_bounce_relight, and the
// compute pipeline it dispatches, shaders/bounce_relight.slang's settle, with its
// own set layout, pool and push block. device.c builds them after the capture
// scratch and tears them down beside it; none without shaderOutputLayer.
//
// ONE RELIGHT, after the begun target's capture passes: nothing on a volume not
// built, and nothing at all, not a barrier, when card 04 says no relight is
// needed. Otherwise every probe marked changed (captured or emptied since the
// last relight) is listed into this slot's list buffer, settle dispatched over
// them, one workgroup each, and the volume marked relit. Barriers either side
// order it after the capture copy and the last frame's reads, and before this
// frame's fragment reads of the validity and moments. Card 09 relights the
// levels between the two and moves the relit mark after them.
//
// THE SET LAYOUT IS THIS FILE'S OWN, set 0 of a compute layout: 0 the normal
// atlas, 1 the moments atlas, 2 the validity, all storage in GENERAL, where
// bounce_volume.c leaves them; 3 the list. One set per frame slot and volume
// (the window 0, target n n), rewritten by every relight: a slot's fence says
// its last use has finished, and a volume may have been rebuilt since.
//
// THE LIST BUFFER is host-visible, coherent and mapped, per slot:
// (targets + 1) × VOE_RENDER_BOUNCE_PROBES_TOTAL words, volume n's band at
// n × the total. A word is a toroidal probe index, bit 31 set when the probe
// holds a picture. Written directly; the submit makes the writes visible.
//
// device->relight_dispatches counts every settle dispatched, for a test to read.
//
// CONSTRAINTS. Listing tests every probe's changed bit, 6912 a relight; a scan
// by word would lift it if a profile names it.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static alignas(uint32_t) const unsigned char bounce_relight_spv[] = {
#embed "bounce_relight.spv"
};

#define BINDINGS 4
#define HOLDS 0x80000000u

// bounce_relight.slang's push block, member for member.
struct settle_push {
	uint32_t first;
	uint32_t count;
	uint32_t reserved[2];
};

static_assert(sizeof(struct settle_push) == 16, "settle push size");
static_assert(VOE_RENDER_BOUNCE_PROBES_TOTAL < HOLDS,
	      "a probe index leaves bit 31 for the holds flag");

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
		.size = sizeof(struct settle_push),
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
			.descriptorType = i == BINDINGS - 1 ?
						  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
						  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
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

static bool create_pipeline(voe_render_device *device)
{
	const VkShaderModuleCreateInfo module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = sizeof(bounce_relight_spv),
		.pCode = (const uint32_t *)bounce_relight_spv,
	};
	VkComputePipelineCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.pName = "settle",
		},
		.layout = device->relight_layout,
	};
	VkShaderModule module;
	VkResult result;

	if (voe_render_vk.create_shader_module(device->device, &module_info, NULL,
					       &module) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateShaderModule failed on bounce_relight.spv");
		return false;
	}
	info.stage.module = module;
	result = voe_render_vk.create_compute_pipelines(device->device,
							VK_NULL_HANDLE, 1, &info,
							NULL, &device->relight_settle);
	voe_render_vk.destroy_shader_module(device->device, module, NULL);
	if (result != VK_SUCCESS) {
		device->relight_settle = VK_NULL_HANDLE;
		VOE_BASE_ERROR("render", "vkCreateComputePipelines failed for the bounce relight");
		return false;
	}
	return true;
}

// The pool, one set per frame slot and volume, and each slot's mapped list.
static bool create_sets_and_lists(voe_render_device *device)
{
	const uint32_t sets = VOE_RENDER_FRAMES_IN_FLIGHT * volume_count(device);
	const VkDescriptorPoolSize sizes[2] = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 * sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sets },
	};
	const VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = sets,
		.poolSizeCount = 2,
		.pPoolSizes = sizes,
	};
	VkDescriptorSetAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorSetCount = 1,
		.pSetLayouts = &device->relight_set_layout,
	};
	const VkDeviceSize lists = (VkDeviceSize)volume_count(device) *
				   VOE_RENDER_BOUNCE_PROBES_TOTAL * sizeof(uint32_t);

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
		if (!voe_render_buffer_build(device, &device->relight_lists[s], lists,
					     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;
		if (voe_render_vk.map_memory(device->device,
					     device->relight_lists[s].memory, 0,
					     VK_WHOLE_SIZE, 0,
					     &device->relight_mapped[s]) != VK_SUCCESS) {
			VOE_BASE_ERROR("render", "vkMapMemory failed on a bounce relight list");
			return false;
		}
	}
	return true;
}

bool voe_render_bounce_relight_startup(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL && device->device != VK_NULL_HANDLE,
			"bounce relight startup before the logical device");
	if (!device->output_layer)
		return true;
	return create_layouts(device) && create_pipeline(device) &&
	       create_sets_and_lists(device);
}

void voe_render_bounce_relight_shutdown(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "bounce relight shutdown on no device");
	for (uint32_t s = 0; s < VOE_RENDER_FRAMES_IN_FLIGHT; s++) {
		voe_render_buffer_teardown(device, &device->relight_lists[s]);
		device->relight_mapped[s] = NULL;
		free(device->relight_sets[s]);
		device->relight_sets[s] = NULL;
	}
	if (device->device == VK_NULL_HANDLE)
		return;
	voe_render_vk.destroy_descriptor_pool(device->device, device->relight_pool,
					      NULL);
	voe_render_vk.destroy_pipeline(device->device, device->relight_settle, NULL);
	voe_render_vk.destroy_pipeline_layout(device->device, device->relight_layout,
					      NULL);
	voe_render_vk.destroy_descriptor_set_layout(device->device,
						    device->relight_set_layout,
						    NULL);
	device->relight_pool = VK_NULL_HANDLE;
	device->relight_settle = VK_NULL_HANDLE;
	device->relight_layout = VK_NULL_HANDLE;
	device->relight_set_layout = VK_NULL_HANDLE;
}

// The bouncing lights this frame's begin placed, as card 04 compares them.
static void begun_lights(const voe_render_device *device,
			 voe_render_bounce_lights *lights)
{
	memset(lights, 0, sizeof(*lights));
	lights->sun = device->bounce_frame.sun;
	lights->sun_bounces = device->bounce_frame.sun_bounces;
	lights->sun_strength = device->bounce_frame.sun_strength;
	lights->lamp_count = device->bounce_frame.points.count;
	memcpy(lights->lamps, device->bounce_lamps,
	       lights->lamp_count * sizeof(lights->lamps[0]));
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

// Points `set` at `volume`'s normal, moments and validity and slot `slot`'s list.
static void write_set(voe_render_device *device,
		      const struct voe_render_bounce_volume *volume,
		      uint32_t slot, VkDescriptorSet set)
{
	const VkDescriptorImageInfo images[3] = {
		{ .imageView = volume->normal.view,
		  .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
		{ .imageView = volume->moments.view,
		  .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
		{ .imageView = volume->validity.view,
		  .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
	};
	const VkDescriptorBufferInfo list = { device->relight_lists[slot].buffer, 0,
					      VK_WHOLE_SIZE };
	VkWriteDescriptorSet writes[BINDINGS];

	for (uint32_t i = 0; i < BINDINGS; i++)
		writes[i] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = set,
			.dstBinding = i,
			.descriptorCount = 1,
			.descriptorType = i < 3 ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE :
						  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pImageInfo = i < 3 ? &images[i] : NULL,
			.pBufferInfo = i < 3 ? NULL : &list,
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

// settle over the `count` probes of volume `index`'s band, between barriers.
static void record_settle(voe_render_device *device,
			  const struct voe_render_bounce_volume *volume,
			  uint32_t index, uint32_t count)
{
	const VkPipelineStageFlags2 compute = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	const VkPipelineStageFlags2 fragment = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	const VkAccessFlags2 storage = VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
				       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	const VkAccessFlags2 sampled = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
	struct voe_render_frame *frame = voe_render_frame_open(device);
	const VkDescriptorSet set = device->relight_sets[device->slot][index];
	const struct settle_push push = {
		.first = index * VOE_RENDER_BOUNCE_PROBES_TOTAL,
		.count = count,
	};

	write_set(device, volume, device->slot, set);
	// The capture copy into the normal atlas, and every earlier read or
	// write of the three images.
	record_barrier(frame->commands,
		       VK_PIPELINE_STAGE_2_COPY_BIT | compute | fragment,
		       VK_ACCESS_2_TRANSFER_WRITE_BIT | storage | sampled, compute,
		       storage);
	voe_render_vk.cmd_bind_pipeline(frame->commands,
					VK_PIPELINE_BIND_POINT_COMPUTE,
					device->relight_settle);
	voe_render_vk.cmd_bind_descriptor_sets(frame->commands,
					       VK_PIPELINE_BIND_POINT_COMPUTE,
					       device->relight_layout, 0, 1, &set,
					       0, NULL);
	voe_render_vk.cmd_push_constants(frame->commands, device->relight_layout,
					 VK_SHADER_STAGE_COMPUTE_BIT, 0,
					 sizeof(push), &push);
	voe_render_vk.cmd_dispatch(frame->commands, count, 1, 1);
	device->relight_dispatches++;
	// The new validity and moments, for the passes and the relight that read them.
	record_barrier(frame->commands, compute, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
		       compute | fragment, storage | sampled);
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
	// Without shaderOutputLayer a begin records nothing to assert on.
	VOE_BASE_ASSERT(device->bounce_begun || !device->output_layer,
			"a bounce relight with no voe_render_bounce_begin this frame");
	if (!device->bounce_begun)
		return;
	volume = voe_render_bounce_volume_of(device, device->bounce_target);
	if (!volume->built)
		return;
	begun_lights(device, &lights);
	if (!voe_render_bounce_probes_relight_needed(&volume->probes, &lights))
		return;

	// The window's volume is 0 and target n's n, its id's index.
	index = device->bounce_target.index;
	VOE_BASE_DEBUG_ASSERT(index < volume_count(device), "a volume with no band");
	count = list_changed(&volume->probes,
			     (uint32_t *)device->relight_mapped[device->slot] +
				     (size_t)index * VOE_RENDER_BOUNCE_PROBES_TOTAL);
	if (count > 0)
		record_settle(device, volume, index, count);
	voe_render_bounce_probes_relit(&volume->probes, &lights);
}
