// The texture encoder (ADR-0486): voe_render_compress_start, _ready and
// _finish, as declared in device.h, running shaders/compress.slang's bc7 and
// bc5 over a caller's RGBA8 chain on the device's own queue.
//
// THE DEVICE'S PART, device->compress: the set layout (binding 0 the texels,
// 1 the blocks, both storage buffers), the pipeline layout with the level's
// push block, and the two pipelines, built under the guard at the first start
// and torn down at close by voe_render_compress_shutdown, which asserts that no
// job is live. A failed build leaves the part empty and the next start tries
// again. `live` counts the jobs started and not finished.
//
// A JOB, struct voe_render_compressing, calloc'd: the input buffer with the
// chain copied in, the output buffer of every level's blocks, a descriptor pool
// of its one set, a command buffer from device->pool of one dispatch a level
// and a fence. Start records and submits it under the guard and does not wait;
// _ready reads the fence; _finish waits on it outside the guard, copies the
// blocks out, and frees the job under the guard again. Each job has its own
// pool, destroyed with it, so no set is ever freed back into a shared one.
//
// WHY BUFFERS AND NOT AN IMAGE. A compute shader cannot write a BC image, and
// the blocks are wanted on the CPU, not drawn: a storage buffer the CPU can map
// is read back directly. The chain goes in the same way, so a job is one
// submission with no staging copy and no layout transition. Both buffers are
// host-visible and coherent: the submit makes the copied-in chain visible, and
// a barrier from the shader's writes to the host's reads ends the recording.
//
// CONSTRAINTS. The job holds its chain and its blocks until _finish: about
// 90 MB in and 22 MB out for a 4096 square chain. The output memory is not asked
// to be host-cached, so reading 22 MB back may be slow on a card whose coherent
// memory is write-combined; asking for HOST_CACHED would lift it. A level's
// side is at most 65535 × 32 texels, the least group count a card allows on one
// axis, far past any image side. A lost device makes _finish write zeros with a
// line, since it hands back nothing.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdlib.h>
#include <string.h>

static alignas(uint32_t) const unsigned char compress_spv[] = {
#embed "compress.spv"
};

// compress.slang's struct compress_push: the level's sides, its first texel and
// its first block.
struct compress_push {
	uint32_t width;
	uint32_t height;
	uint32_t texel;
	uint32_t block;
};

struct voe_render_compressing {
	struct voe_render_buffer texels;
	struct voe_render_buffer blocks;
	void *blocks_mapped;
	VkDeviceSize block_bytes;
	VkDescriptorPool pool;
	VkCommandBuffer commands;
	VkFence fence;
};

static uint32_t level_side(uint32_t side, uint32_t level)
{
	return side >> level > 0 ? side >> level : 1;
}

// The part's handles destroyed and nulled; any of them may be VK_NULL_HANDLE.
static void destroy_part(voe_render_device *device)
{
	struct voe_render_compress *part = &device->compress;

	voe_render_vk.destroy_pipeline(device->device, part->bc7, NULL);
	voe_render_vk.destroy_pipeline(device->device, part->bc5, NULL);
	voe_render_vk.destroy_pipeline_layout(device->device, part->layout, NULL);
	voe_render_vk.destroy_descriptor_set_layout(device->device,
						    part->set_layout, NULL);
	part->bc7 = VK_NULL_HANDLE;
	part->bc5 = VK_NULL_HANDLE;
	part->layout = VK_NULL_HANDLE;
	part->set_layout = VK_NULL_HANDLE;
}

static bool create_layouts(voe_render_device *device)
{
	struct voe_render_compress *part = &device->compress;
	const VkDescriptorSetLayoutBinding bindings[2] = {
		{ .binding = 0,
		  .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		  .descriptorCount = 1,
		  .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
		{ .binding = 1,
		  .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		  .descriptorCount = 1,
		  .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
	};
	const VkDescriptorSetLayoutCreateInfo set_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 2,
		.pBindings = bindings,
	};
	const VkPushConstantRange push = {
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.size = sizeof(struct compress_push),
	};
	const VkPipelineLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &part->set_layout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push,
	};

	if (voe_render_vk.create_descriptor_set_layout(device->device, &set_info,
						       NULL, &part->set_layout) !=
	    VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateDescriptorSetLayout failed for the texture encoder");
		return false;
	}
	if (voe_render_vk.create_pipeline_layout(device->device, &layout_info,
						 NULL, &part->layout) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreatePipelineLayout failed for the texture encoder");
		return false;
	}
	return true;
}

// bc7 and bc5, from the one module.
static bool create_pipelines(voe_render_device *device)
{
	struct voe_render_compress *part = &device->compress;
	static const char *const entries[2] = { "bc7", "bc5" };
	const VkShaderModuleCreateInfo module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = sizeof(compress_spv),
		.pCode = (const uint32_t *)compress_spv,
	};
	VkComputePipelineCreateInfo infos[2];
	VkPipeline pipelines[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	VkShaderModule module;
	VkResult result;

	if (voe_render_vk.create_shader_module(device->device, &module_info, NULL,
					       &module) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateShaderModule failed on compress.spv");
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
			.layout = part->layout,
		};
	result = voe_render_vk.create_compute_pipelines(device->device,
							VK_NULL_HANDLE, 2, infos,
							NULL, pipelines);
	voe_render_vk.destroy_shader_module(device->device, module, NULL);
	part->bc7 = pipelines[0];
	part->bc5 = pipelines[1];
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateComputePipelines failed for the texture encoder");
		return false;
	}
	voe_render_debug_name(device, VK_OBJECT_TYPE_PIPELINE,
			      (uint64_t)pipelines[0], "compress bc7 pipeline");
	voe_render_debug_name(device, VK_OBJECT_TYPE_PIPELINE,
			      (uint64_t)pipelines[1], "compress bc5 pipeline");
	return true;
}

// The part, built once; on failure emptied so the next start tries again.
static bool build_part(voe_render_device *device)
{
	if (device->compress.bc7 != VK_NULL_HANDLE)
		return true;
	if (create_layouts(device) && create_pipelines(device))
		return true;
	destroy_part(device);
	return false;
}

static bool build_mapped(voe_render_device *device,
			 struct voe_render_buffer *buffer, VkDeviceSize size,
			 void **mapped, const char *name)
{
	if (!voe_render_buffer_build(device, buffer, size,
				     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				     name))
		return false;
	if (voe_render_vk.map_memory(device->device, buffer->memory, 0,
				     VK_WHOLE_SIZE, 0, mapped) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkMapMemory failed on a texture encoder buffer");
		return false;
	}
	return true;
}

// The job's buffers with the chain copied in, its pool and set naming them, its
// command buffer and its fence.
static bool build_job(voe_render_device *device, voe_render_compressing *job,
		      uint32_t width, uint32_t height, uint32_t level_count,
		      const uint8_t *rgba, VkDescriptorSet *set)
{
	VkDeviceSize texels = 0, blocks = 0;
	void *texels_mapped = NULL;
	const VkDescriptorPoolSize size = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 };
	const VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.poolSizeCount = 1,
		.pPoolSizes = &size,
	};
	VkDescriptorSetAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorSetCount = 1,
		.pSetLayouts = &device->compress.set_layout,
	};
	const VkCommandBufferAllocateInfo commands_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = device->pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	const VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	};

	for (uint32_t i = 0; i < level_count; i++) {
		const uint32_t w = level_side(width, i), h = level_side(height, i);

		texels += (VkDeviceSize)w * h;
		blocks += (VkDeviceSize)((w + 3) / 4) * ((h + 3) / 4);
	}
	job->block_bytes = blocks * 16;
	if (!build_mapped(device, &job->texels, texels * 4, &texels_mapped,
			  "compress texels") ||
	    !build_mapped(device, &job->blocks, job->block_bytes,
			  &job->blocks_mapped, "compress blocks"))
		return false;
	memcpy(texels_mapped, rgba, (size_t)texels * 4);
	if (voe_render_vk.create_descriptor_pool(device->device, &pool_info, NULL,
						 &job->pool) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateDescriptorPool failed for a texture encoder job");
		return false;
	}
	allocate.descriptorPool = job->pool;
	if (voe_render_vk.allocate_descriptor_sets(device->device, &allocate,
						   set) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkAllocateDescriptorSets failed for a texture encoder job");
		return false;
	}
	const VkDescriptorBufferInfo buffers[2] = {
		{ job->texels.buffer, 0, VK_WHOLE_SIZE },
		{ job->blocks.buffer, 0, VK_WHOLE_SIZE },
	};
	const VkWriteDescriptorSet write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = *set,
		.descriptorCount = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = buffers,
	};
	voe_render_vk.update_descriptor_sets(device->device, 1, &write, 0, NULL);
	if (voe_render_vk.allocate_command_buffers(device->device, &commands_info,
						   &job->commands) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkAllocateCommandBuffers failed for a texture encoder job");
		return false;
	}
	if (voe_render_vk.create_fence(device->device, &fence_info, NULL,
				       &job->fence) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateFence failed for a texture encoder job");
		return false;
	}
	return true;
}

// One dispatch a level, then the blocks made visible to the host; submitted.
static bool record_and_submit(voe_render_device *device,
			      voe_render_compressing *job,
			      voe_render_blocks blocks, uint32_t width,
			      uint32_t height, uint32_t level_count,
			      VkDescriptorSet set)
{
	const struct voe_render_compress *part = &device->compress;
	const VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	const VkMemoryBarrier2 visible = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
		.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
	};
	const VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &visible,
	};
	const VkCommandBufferSubmitInfo submit_commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = job->commands,
	};
	const VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &submit_commands,
	};
	struct compress_push push = { 0 };

	if (voe_render_vk.begin_command_buffer(job->commands, &begin) !=
	    VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkBeginCommandBuffer failed for a texture encoder job");
		return false;
	}
	voe_render_vk.cmd_bind_pipeline(job->commands,
					VK_PIPELINE_BIND_POINT_COMPUTE,
					blocks == VOE_RENDER_BLOCKS_BC7 ? part->bc7 :
									  part->bc5);
	voe_render_vk.cmd_bind_descriptor_sets(job->commands,
					       VK_PIPELINE_BIND_POINT_COMPUTE,
					       part->layout, 0, 1, &set, 0, NULL);
	for (uint32_t i = 0; i < level_count; i++) {
		const uint32_t across = (level_side(width, i) + 3) / 4;
		const uint32_t down = (level_side(height, i) + 3) / 4;

		push.width = level_side(width, i);
		push.height = level_side(height, i);
		voe_render_vk.cmd_push_constants(job->commands, part->layout,
						 VK_SHADER_STAGE_COMPUTE_BIT, 0,
						 sizeof push, &push);
		voe_render_vk.cmd_dispatch(job->commands, (across + 7) / 8,
					   (down + 7) / 8, 1);
		push.texel += push.width * push.height;
		push.block += across * down;
	}
	voe_render_vk.cmd_pipeline_barrier2(job->commands, &dependency);
	if (voe_render_vk.end_command_buffer(job->commands) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkEndCommandBuffer failed for a texture encoder job");
		return false;
	}
	if (voe_render_vk.queue_submit2(device->queue, 1, &submit, job->fence) !=
	    VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkQueueSubmit2 failed for a texture encoder job");
		return false;
	}
	return true;
}

// Everything a job holds but the struct itself, safe on a job built part way.
// Under the guard: device->pool is shared.
static void free_job(voe_render_device *device, voe_render_compressing *job)
{
	if (job->commands != VK_NULL_HANDLE)
		voe_render_vk.free_command_buffers(device->device, device->pool, 1,
						   &job->commands);
	voe_render_vk.destroy_fence(device->device, job->fence, NULL);
	voe_render_vk.destroy_descriptor_pool(device->device, job->pool, NULL);
	voe_render_buffer_teardown(device, &job->texels);
	voe_render_buffer_teardown(device, &job->blocks);
}

voe_render_compressing *voe_render_compress_start(voe_render_device *device,
						  voe_render_blocks blocks,
						  uint32_t width, uint32_t height,
						  uint32_t level_count,
						  const uint8_t *rgba,
						  voe_base_error *error)
{
	VOE_BASE_ASSERT(device != NULL && device->device != VK_NULL_HANDLE &&
				rgba != NULL && error != NULL,
			"compress start needs a device, a chain and an error");
	VOE_BASE_ASSERT(blocks == VOE_RENDER_BLOCKS_BC7 ||
				blocks == VOE_RENDER_BLOCKS_BC5,
			"compress start into a block format there is none of");
	VOE_BASE_ASSERT(width > 0 && height > 0 && level_count > 0 &&
				level_count <= voe_render_texture_full_levels(
						       width, height),
			"compress start: no picture, or more levels than its full chain");
	voe_render_compressing *job = calloc(1, sizeof *job);
	VkDescriptorSet set = VK_NULL_HANDLE;
	bool started;

	if (job == NULL) {
		VOE_BASE_ERROR("render", "out of memory for a texture encoder job");
		*error = VOE_BASE_ERROR_REFUSED;
		return NULL;
	}
	voe_render_device_guard_take(device);
	started = build_part(device) &&
		  build_job(device, job, width, height, level_count, rgba,
			    &set) &&
		  record_and_submit(device, job, blocks, width, height,
				    level_count, set);
	if (started)
		device->compress.live++;
	else
		free_job(device, job);
	voe_render_device_guard_give(device);
	if (!started) {
		free(job);
		*error = VOE_BASE_ERROR_REFUSED;
		return NULL;
	}
	return job;
}

bool voe_render_compress_ready(voe_render_device *device,
			       const voe_render_compressing *job)
{
	VOE_BASE_ASSERT(device != NULL && job != NULL,
			"compress ready asked of no device or no job");
	return voe_render_vk.get_fence_status(device->device, job->fence) ==
	       VK_SUCCESS;
}

void voe_render_compress_finish(voe_render_device *device,
				voe_render_compressing *job, uint8_t *out)
{
	VOE_BASE_ASSERT(device != NULL && job != NULL,
			"compress finish of no device or no job");
	const bool done = voe_render_vk.wait_for_fences(device->device, 1,
							&job->fence, VK_TRUE,
							UINT64_MAX) == VK_SUCCESS;

	if (!done)
		VOE_BASE_ERROR("render", "a texture encoder job never finished; its blocks are zeros");
	if (out != NULL) {
		if (done)
			memcpy(out, job->blocks_mapped, (size_t)job->block_bytes);
		else
			memset(out, 0, (size_t)job->block_bytes);
	}
	voe_render_device_guard_take(device);
	free_job(device, job);
	device->compress.live--;
	voe_render_device_guard_give(device);
	free(job);
}

void voe_render_compress_shutdown(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "compress shutdown on no device");
	VOE_BASE_ASSERT(device->compress.live == 0,
			"a device closed with a texture encoder job not finished");
	if (device->device == VK_NULL_HANDLE)
		return;
	destroy_part(device);
}
