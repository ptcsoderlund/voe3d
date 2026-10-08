// What the relight (bounce_relight.c) runs on: the compute pipelines of
// shaders/bounce_relight.slang's settle, relight and sum, with their own set
// layout, pool and push block, and each slot's list and record buffers. Prepare
// (pipeline.c) builds them through voe_render_bounce_relight_startup as its
// last step and device.c tears them down through
// voe_render_bounce_relight_shutdown beside the capture scratch, safely when
// they were never built; none without shaderOutputLayer.
//
// THE SET LAYOUT IS THE RELIGHT'S OWN, set 0 of a compute layout: 0 the normal
// atlas, 1 the moments, 2 the validity, storage; 3 the list; 4 the seven grids'
// 21 images, storage; 5 the six levels' 18, 6 the validity and 7 the moments,
// sampled through the device's bounce and moments samplers; 8 the albedo atlas,
// sampled; 9 every layer of the slot's bounce shadow map (bounce_shadow.c's)
// and 10 its point shadow maps, through the shadow sampler; 11 its volume's
// region of the slot's record buffer. The volume's images rest in GENERAL,
// the maps where the shader reads them. One set per frame slot and volume, every
// volume of the window and of each target, indexed by the volume's descriptor
// index d (voe_render_bounce_volume_index), rewritten by every relight: a
// slot's fence says its last use has finished, and a volume may have been
// rebuilt since.
//
// PER SLOT, host-visible, coherent and mapped, written directly, the submit
// making them visible: the list buffer, voe_render_relight_volume_count ×
// VOE_RENDER_BOUNCE_PROBES_TOTAL words, volume d's band at d × the total, a
// word as bounce_relight.h lays it out; and the
// record buffer, a region per volume device->relight_record_stride apart (the
// record rounded up to the card's uniform offset alignment), volume d's at d ×
// the stride, binding 11 of its set naming that region alone. One record per
// volume and not per slot (ADR-0330 point 2): the CPU writes it at the relight,
// so two volumes relit in one frame would both read the last one's. A struct
// voe_render_relight_record is the uniform the relight reads: the begun sun, the
// bounce shadow map's view × projection, its texel (the box's width over
// VOE_RENDER_BOUNCE_SHADOW_TEXELS) and whether this frame drew it, the sun's
// strength, the volume's placement (cell, corner and its begin's spacing), the
// bouncing lamps and the begun light blockers (ADR-0347 point 4) with their
// kinds and the sun's mask (ADR-0350 point 2); then the further suns that
// bounce (ADR-0357 point 4), each with its own layer's map, texel and drawn
// flag (layer i + 1 for the i-th), strength, mask and bounces.
#include "bounce_relight.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdlib.h>

static alignas(uint32_t) const unsigned char bounce_relight_spv[] = {
#embed "bounce_relight.spv"
};

static_assert(sizeof(struct voe_render_relight_push) == 32,
	      "relight push size");
static_assert(VOE_RENDER_BOUNCE_PROBES_TOTAL <= VOE_RENDER_RELIGHT_PROBE + 1,
	      "a probe index fits bits 0–15 of a list word");
static_assert(VOE_RENDER_BOUNCE_FADE <= VOE_RENDER_RELIGHT_READY,
	      "a readiness fits bits 16–20 of a list word");
static_assert(VOE_RENDER_BOUNCES_MAX == 3, "three chains of six levels");

static bool create_layouts(voe_render_device *device)
{
	VkDescriptorSetLayoutBinding bindings[VOE_RENDER_RELIGHT_BINDINGS];
	VkDescriptorSetLayoutCreateInfo set_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = VOE_RENDER_RELIGHT_BINDINGS,
		.pBindings = bindings,
	};
	const VkPushConstantRange push = {
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.size = sizeof(struct voe_render_relight_push),
	};
	VkPipelineLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push,
	};

	for (uint32_t i = 0; i < VOE_RENDER_RELIGHT_BINDINGS; i++)
		bindings[i] = (VkDescriptorSetLayoutBinding){
			.binding = i,
			.descriptorType = voe_render_relight_binding_types[i],
			.descriptorCount = voe_render_relight_binding_counts[i],
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
	voe_render_debug_name(device, VK_OBJECT_TYPE_PIPELINE,
			      (uint64_t)pipelines[0], "relight settle pipeline");
	voe_render_debug_name(device, VK_OBJECT_TYPE_PIPELINE,
			      (uint64_t)pipelines[1], "relight levels pipeline");
	voe_render_debug_name(device, VK_OBJECT_TYPE_PIPELINE,
			      (uint64_t)pipelines[2], "relight sum pipeline");
	return true;
}

// One slot's mapped buffer of `size` bytes and `usage`.
static bool build_mapped(voe_render_device *device,
			 struct voe_render_buffer *buffer, VkDeviceSize size,
			 VkBufferUsageFlags usage, void **mapped,
			 const char *name)
{
	if (!voe_render_buffer_build(device, buffer, size, usage,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				     name))
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
	const uint32_t volumes = voe_render_relight_volume_count(device);
	const uint32_t sets = VOE_RENDER_FRAMES_IN_FLIGHT * volumes;
	const VkDescriptorPoolSize sizes[5] = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		  (3 + VOE_RENDER_RELIGHT_GRID_IMAGES) * sets },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		  (VOE_RENDER_RELIGHT_LEVEL_IMAGES + 4) * sets },
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
	const VkDeviceSize lists = (VkDeviceSize)volumes *
				   VOE_RENDER_BOUNCE_PROBES_TOTAL * sizeof(uint32_t);

	device->relight_record_stride = record_stride(device);
	if (voe_render_vk.create_descriptor_pool(device->device, &pool_info, NULL,
						 &device->relight_pool) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateDescriptorPool failed for the bounce relight");
		return false;
	}
	allocate.descriptorPool = device->relight_pool;
	for (uint32_t s = 0; s < VOE_RENDER_FRAMES_IN_FLIGHT; s++) {
		device->relight_sets[s] = calloc(volumes,
						 sizeof(*device->relight_sets[s]));
		if (device->relight_sets[s] == NULL) {
			VOE_BASE_ERROR("render", "out of memory for the bounce relight sets");
			return false;
		}
		for (uint32_t v = 0; v < volumes; v++)
			if (voe_render_vk.allocate_descriptor_sets(device->device,
								   &allocate,
								   &device->relight_sets[s][v]) !=
			    VK_SUCCESS) {
				VOE_BASE_ERROR("render", "vkAllocateDescriptorSets failed for the bounce relight");
				return false;
			}
		if (!build_mapped(device, &device->relight_lists[s], lists,
				  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				  &device->relight_mapped[s],
				  "bounce relight lists") ||
		    !build_mapped(device, &device->relight_records[s],
				  volumes * device->relight_record_stride,
				  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				  &device->relight_records_mapped[s],
				  "bounce relight records"))
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
