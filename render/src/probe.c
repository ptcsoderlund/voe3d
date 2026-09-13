// The matrix probe: a pipeline that reads the engine's uniform buffer and writes
// three of its elements out as colour, so that a test can find out what layout
// the shader compiler actually used. See shaders/matrix_probe.slang for what the
// three elements are and why those three.
//
// IT EXISTS BECAUSE A FLAG CANNOT OTHERWISE BE CHECKED. cmake/voe.cmake invokes
// slangc with -matrix-layout-row-major. With it, a float4x4 in a shader has the
// layout voe_math_float4x4 has in C; without it the same sixteen floats are read
// column-major, every transform in the engine comes out transposed, and nothing
// fails to compile. render/tests/matrix.c uploads a known matrix and requires
// this shader to report it back, so removing the flag fails a test instead of
// quietly transposing the world.
//
// THE PIPELINE IS BUILT ON DEMAND AND OWNED BY WHOEVER ASKED. Only the test asks.
// A shipping device that built this at startup would be paying for a pipeline
// nothing ever draws, which is exactly the surface rule 10 exists to prevent —
// so there is a function that makes one and the caller destroys it.
//
// THE COMPILED SHADER IS IN THE BINARY WHETHER OR NOT ANYONE ASKS, AND THAT IS
// NOT AVOIDABLE HERE. --embed-dir is private to this folder's library
// (cmake/voe.cmake), so a test cannot #embed a .spv of its own and the bytes have
// to be in a file under src/. It is a few hundred bytes of a shader with one
// instruction worth of work in it, and the alternative is making the embed
// directory public so that anything could embed anything.
//
// IT SHARES THE DEVICE'S PIPELINE LAYOUT ON PURPOSE. That is what makes this a
// test of the path the engine really uploads through: the same descriptor set
// layout, the same binding, the same buffer the cube reads. A layout of its own
// would be a second path, and a second path is the thing that can pass while the
// real one is broken.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

// alignas because vkCreateShaderModule takes a const uint32_t *, and #embed can
// only fill an array of bytes. Same reasoning as the cube's shader in device.c.
static alignas(uint32_t) const unsigned char matrix_probe_spv[] = {
#embed "matrix_probe.spv"
};

#define PROBE_VERTEX_ENTRY "voe_render_matrix_probe_vertex"
#define PROBE_FRAGMENT_ENTRY "voe_render_matrix_probe_fragment"

VkPipeline voe_render_probe_pipeline_new(voe_render_device *device)
{
	VkShaderModuleCreateInfo module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = sizeof(matrix_probe_spv),
		.pCode = (const uint32_t *)matrix_probe_spv,
	};
	VkShaderModule module = VK_NULL_HANDLE;
	VkPipelineShaderStageCreateInfo stages[2];
	// Nothing is fed in: the three vertices are constants in the shader and
	// the draw counts them. Present and empty because Vulkan requires one.
	VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	};
	VkPipelineInputAssemblyStateCreateInfo assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	};
	VkPipelineViewportStateCreateInfo viewport = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};
	// CULLING OFF, AND THAT IS THE POINT OF A SEPARATE PIPELINE. What is
	// under test is a matrix, so the probe must not be able to vanish for
	// any other reason. Winding and the front-face constant are
	// render/tests/offscreen.c's claim and they are not repeated here.
	VkPipelineRasterizationStateCreateInfo raster = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.lineWidth = 1.0f,
	};
	VkPipelineMultisampleStateCreateInfo multisample = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};
	VkPipelineColorBlendAttachmentState attachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
				  VK_COLOR_COMPONENT_G_BIT |
				  VK_COLOR_COMPONENT_B_BIT |
				  VK_COLOR_COMPONENT_A_BIT,
	};
	VkPipelineColorBlendStateCreateInfo blend = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &attachment,
	};
	VkDynamicState dynamic_states[2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo dynamic = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamic_states,
	};
	// No depth attachment format and no depth-stencil state, which has to
	// match the rendering voe_render_probe_draw begins: a pipeline that
	// declared a depth format and a rendering that attached none is invalid.
	VkPipelineRenderingCreateInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &device->format.format,
	};
	VkGraphicsPipelineCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &rendering,
		.stageCount = 2,
		.pStages = stages,
		.pVertexInputState = &vertex_input,
		.pInputAssemblyState = &assembly,
		.pViewportState = &viewport,
		.pRasterizationState = &raster,
		.pMultisampleState = &multisample,
		.pColorBlendState = &blend,
		.pDynamicState = &dynamic,
	};
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkResult result;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "building a probe without a device");
	VOE_BASE_DEBUG_ASSERT(device->layout != VK_NULL_HANDLE,
			      "building a probe before there is a pipeline layout");

	if (voe_render_vk.create_shader_module(device->device, &module_info,
					       NULL, &module) != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateShaderModule failed on matrix_probe.spv");
		return VK_NULL_HANDLE;
	}

	stages[0] = (VkPipelineShaderStageCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = module,
		.pName = PROBE_VERTEX_ENTRY,
	};
	stages[1] = (VkPipelineShaderStageCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = module,
		.pName = PROBE_FRAGMENT_ENTRY,
	};

	// The cube's layout, which names the descriptor set the probe reads.
	info.layout = device->layout;

	result = voe_render_vk.create_graphics_pipelines(device->device,
							 VK_NULL_HANDLE, 1,
							 &info, NULL, &pipeline);

	// The module is the compiler's input and the pipeline has finished
	// reading it, so it goes away here whether or not the pipeline was made.
	voe_render_vk.destroy_shader_module(device->device, module, NULL);

	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateGraphicsPipelines failed for the matrix probe (VkResult %d)",
			       (int)result);
		return VK_NULL_HANDLE;
	}
	return pipeline;
}

void voe_render_probe_draw(voe_render_device *device,
			   const struct voe_render_frame *frame,
			   VkPipeline pipeline, VkViewport viewport)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = frame->target.colour.image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};
	// LOAD_OP_DONT_CARE, not CLEAR: the probe covers every pixel of the
	// target, so a clear would be work whose every result is overwritten.
	// It also means a pixel that somehow was not covered holds whatever was
	// there before rather than a colour that looks deliberate.
	VkRenderingAttachmentInfo colour = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = frame->target.colour.view,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	};
	// No depth attachment, matching the pipeline above.
	VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .extent = device->resolution },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colour,
	};
	VkRect2D scissor = { .extent = device->resolution };

	VOE_BASE_DEBUG_ASSERT(device != NULL, "drawing a probe with a NULL device");
	VOE_BASE_DEBUG_ASSERT(pipeline != VK_NULL_HANDLE,
			      "drawing a probe with no pipeline");
	VOE_BASE_DEBUG_ASSERT(frame->target.colour.image != VK_NULL_HANDLE,
			      "drawing a probe into a frame slot that has no colour target");

	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);

	voe_render_vk.cmd_begin_rendering(frame->commands, &rendering);
	voe_render_vk.cmd_set_viewport(frame->commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(frame->commands, 0, 1, &scissor);
	voe_render_vk.cmd_bind_pipeline(frame->commands,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipeline);
	// The same descriptor the cube reads, holding whatever the caller put
	// there. This writes nothing to it.
	voe_render_vk.cmd_bind_descriptor_sets(frame->commands,
					       VK_PIPELINE_BIND_POINT_GRAPHICS,
					       device->layout, 0, 1,
					       &frame->descriptor, 0, NULL);
	voe_render_vk.cmd_draw(frame->commands, 3, 1, 0, 0);
	voe_render_vk.cmd_end_rendering(frame->commands);

	// Left ready to be copied out of, exactly as voe_render_frame_draw
	// leaves it, so that a caller can use the same copy either way.
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
}
