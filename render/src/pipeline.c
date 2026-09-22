// The two mesh pipelines, solid and blended, and the pipeline layout every
// pipeline in this folder shares. A step of startup; device.c's open_device
// calls voe_render_pipelines_create once, after the descriptors — see startup.h.
//
// THE SHADER IS IN THIS FILE, AS BYTES. slangc compiles shaders/draw.slang into
// the build tree and #embed puts the result in the binary below; nothing is read
// from disk at run time and there is no shader path to get wrong on someone
// else's machine. cmake/voe.cmake makes every source in this folder depend on
// the .spv, so this file needed no build edit of its own.
//
// THE PIPELINE NAMES THINGS OTHER STARTUP FILES OWN. Its layout names the
// descriptor set layout descriptors.c builds, and its vertex input describes
// voe_render_vertex — which is why voe_render_descriptors_build runs before this
// in open_device and not after it.
//
// THERE ARE TWO OF THEM AND THEY DIFFER IN THREE LINES. The solid one writes
// depth and does not blend; the blended one tests depth the same way, writes
// none, and blends premultiplied. Everything else — the shader module, the
// vertex input, the raster state, the layout — is one description built once and
// handed to both, which is what keeps the two from drifting apart: see
// create_pipeline's `blended` parameter, which is the whole of the difference.
//
// A THIRD PIPELINE IS NOT BUILT HERE AND IT IS NOT A VARIANT OF THESE TWO.
// element.c builds it: no vertex input at all, a triangle strip, nothing culled
// and its own shader, which is four differences and nothing left of the shared
// description. What it does share is the layout below, which is why
// voe_render_element_startup runs after this in open_device and why this file's
// push constant range is a matrix wide rather than a word.
//
// DEPTH IS SET UP HERE AND IT RUNS BACKWARDS. GREATER, not LESS, because the
// near plane is at 1.0 and the far plane at 0.0. The clear that goes with it is
// in frame.c and the projection matrix that produces those planes is 3d's;
// change any one of the three alone and the picture is wrong in a way that still
// looks plausible.
#include "startup.h"

#include <base/assert.h>
#include <base/report.h>

#include <stddef.h>

// ------------------------------------------------------------------- pipeline

// The compiled shader, in the binary. slangc writes draw.spv into the build tree
// and cmake/voe.cmake puts that directory on this file's include path, so the
// quoted name below resolves to a generated file and never to one in the source
// tree. There is no fallback path and no file to ship beside the binary.
//
// alignas because vkCreateShaderModule takes a const uint32_t *, and #embed can
// only fill an array of bytes. A char array is aligned for a char; handing a
// misaligned pointer to the driver is undefined behaviour that happens to work
// until the day it does not.
static alignas(uint32_t) const unsigned char draw_spv[] = {
#embed "draw.spv"
};

// Both entry points live in the one module above, spelled exactly as the shader
// spells them — see -fvk-use-entrypoint-name in cmake/voe.cmake, which is what
// keeps these two strings true.
#define DRAW_VERTEX_ENTRY "voe_render_draw_vertex"
#define DRAW_FRAGMENT_ENTRY "voe_render_draw_fragment"

// Both pipelines, from one description. `blended` is the only thing that differs
// between the two and the three lines it touches are marked below.
static bool create_pipeline(voe_render_device *device, bool blended,
			    VkPipeline *out)
{
	VkShaderModuleCreateInfo module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = sizeof(draw_spv),
		.pCode = (const uint32_t *)draw_spv,
	};
	VkShaderModule module = VK_NULL_HANDLE;
	VkPipelineShaderStageCreateInfo stages[2];
	// One buffer, read one vertex at a time. The stride is the struct's own
	// size rather than a number written out, so a field added to
	// voe_render_vertex cannot leave this behind.
	VkVertexInputBindingDescription binding = {
		.binding = 0,
		.stride = sizeof(voe_render_vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};
	// THESE THREE LOCATIONS AND draw.slang's THREE vk::location NUMBERS ARE ONE
	// FACT IN TWO PLACES. Both are stated rather than counted, and both
	// offsets come from offsetof rather than from adding up sizes — which is
	// what makes reordering the struct's fields harmless and renaming one of
	// them a compile error instead of a wrong picture.
	VkVertexInputAttributeDescription attributes[3] = {
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(voe_render_vertex, position),
		},
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(voe_render_vertex, normal),
		},
		{
			.location = 2,
			.binding = 0,
			// Two floats, not three: a texture coordinate.
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(voe_render_vertex, uv),
		},
	};
	VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding,
		.vertexAttributeDescriptionCount = 3,
		.pVertexAttributeDescriptions = attributes,
	};
	VkPipelineInputAssemblyStateCreateInfo assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	};
	// One of each, and what they are is decided per frame — see the dynamic
	// state below. Counts here, values in frame.c.
	VkPipelineViewportStateCreateInfo viewport = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};
	// FRONT FACE IS COUNTER-CLOCKWISE, WHICH IS THE SAME WORD THE WORLD USES,
	// AND THAT IS THE WHOLE POINT OF THE FLIP. This engine winds front faces
	// counter-clockwise with +Y up, as glTF does. Vulkan decides facing from
	// framebuffer coordinates, so without the negative viewport height it
	// would want the opposite constant — the flip is what puts the two
	// systems into agreement, and once they agree the constant here is
	// spelled the way the convention is spelled and no translation happens
	// anywhere.
	//
	// IT SAID CLOCKWISE ONCE, AND THAT WAS THE DOUBLE NEGATIVE CLAUDE.md
	// WARNS ABOUT. The reasoning written here was that the flip reverses the
	// winding so the constant must be reversed too; the flip is what removes
	// the reversal, and applying both left a front face Vulkan called a back
	// one. Nothing culled, so nothing showed it, which is exactly the failure
	// mode the rule about proving this with a test names.
	//
	// THESE TWO LINES AND THE VIEWPORT'S SIGN ARE ONE FACT IN THREE PLACES,
	// AND render/tests/offscreen.c IS WHAT HOLDS THEM TOGETHER. That test
	// draws a cube into an offscreen image through the engine's own viewport
	// and reads the centre pixel back, then draws it again through the mirror
	// of that viewport — every face wound the other way — and requires the
	// first to show the near face and the second to show the far one. The
	// cube's near and far faces carry the same picture the other way round
	// by construction, so the two cases cannot be confused. Change either line here, or the
	// sign in voe_render_frame_viewport, and it fails. Change all three and it
	// still fails, which is the point: flipping twice looks exactly like
	// flipping none until something is culled.
	VkPipelineRasterizationStateCreateInfo raster = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.lineWidth = 1.0f,
	};
	VkPipelineMultisampleStateCreateInfo multisample = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};
	// GREATER, AND THAT IS THE WHOLE OF THIS ENGINE'S REVERSED DEPTH ON THE
	// PIPELINE'S SIDE. A fragment survives when it is *nearer*, and nearer
	// means a larger depth here, because voe_3d_projection puts the near
	// plane at 1.0 and the far plane at 0.0. LESS_OR_EQUAL is what every
	// tutorial writes and it would keep the farthest fragment instead — on a
	// convex shape like a cube that still draws something, which is why the
	// card asks for the comparison to be flipped on purpose and looked at.
	//
	// depthBoundsTestEnable stays off: it clips against a depth range and
	// this engine has nothing that wants that. The two bounds below are the
	// full range and are ignored while the test is off; they are stated so
	// that a reader does not have to wonder whether a zero here means the
	// near plane.
	//
	// THE BLENDED PIPELINE TESTS DEPTH AND DOES NOT WRITE IT, AND NEITHER
	// HALF OF THAT IS OPTIONAL. The test stays on so that a see-through
	// thing behind a solid one is still hidden by it. The write goes off so
	// that two see-through things do not hide each other — which is what
	// makes the caller's furthest-first order load-bearing rather than
	// cosmetic. Leave the write on and the sort appears to work while doing
	// nothing at all.
	VkPipelineDepthStencilStateCreateInfo depth = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = blended ? VK_FALSE : VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_GREATER,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
	};
	// THE BLEND IS PREMULTIPLIED AND THAT IS THE ENGINE'S CONTRACT, NOT A
	// TASTE. Source factor ONE, destination factor ONE_MINUS_SRC_ALPHA, for
	// colour and for alpha both, because a fragment leaves the shader with
	// its colour already multiplied by its own alpha — draw.slang's last act.
	// The textbook non-premultiplied pair (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)
	// would multiply by alpha a second time and everything see-through would
	// come out too dark.
	//
	// THE ALPHA CHANNEL IS BLENDED THE SAME WAY AND NOT LEFT ALONE. The
	// target's alpha is what a later compositing step would read; keeping it
	// consistent costs nothing and an inconsistent one is invisible until
	// something reads it.
	//
	// The solid pipeline writes rather than blends: everything it draws is
	// fully solid and there is nothing underneath it but the clear.
	VkPipelineColorBlendAttachmentState attachment = {
		.blendEnable = blended ? VK_TRUE : VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.alphaBlendOp = VK_BLEND_OP_ADD,
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
	// So that a resize rebuilds the targets and the swapchain and nothing
	// else. A pipeline baked at one size would have to be built again on
	// every resize, which is a lot of driver work to say a number that
	// changed.
	VkDynamicState dynamic_states[2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo dynamic = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamic_states,
	};
	// Dynamic rendering has no render pass, so the pipeline is told the
	// attachment format here instead. It is the target's, which is the
	// surface's, chosen once and unchanged by a resize — which is what makes
	// this a startup decision and not a per-resize one. The day the target
	// stops sharing the swapchain's format, this line follows the target and
	// not the screen.
	// Dynamic rendering has no render pass, so both attachment formats are
	// declared here instead. They have to match what frame.c attaches, and a
	// depth format declared with no depth attachment — or the other way
	// round — is invalid rather than merely wrong.
	VkPipelineRenderingCreateInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &device->format.format,
		.depthAttachmentFormat = VOE_RENDER_DEPTH_FORMAT,
	};
	// One set holding everything the shader reads, and one push constant
	// range holding the one number that differs between two draws in a
	// frame. descriptors.c made the set layout, which is why it has to have
	// run before this function does; the range is described here because it
	// belongs to the pipeline layout and to nothing else.
	//
	// BOTH STAGES, because both read the object's record: the vertex stage
	// wants its world matrix and the fragment stage wants the shading index
	// in it. A range that named one stage would make the other's read
	// invalid.
	//
	// IT IS A MATRIX WIDE THOUGH A MESH DRAW STILL PUSHES FOUR BYTES, AND
	// THAT IS THE ELEMENT PIPELINE'S DOING. shaders/elements.slang pushes a
	// sixty-four-byte surface transform through this same range, because
	// that pipeline shares this layout — and it shares it so that the
	// descriptor set frame.c binds as a pass opens stays bound
	// across an element draw. Two layouts differing only in their push
	// constant ranges are incompatible, and binding a pipeline with an
	// incompatible layout disturbs the set for everything drawn afterwards.
	//
	// THE TWO BLOCKS ALIAS AND NEITHER EVER READS THE OTHER'S BYTES. Both
	// start at offset nought, and each pipeline pushes its own immediately
	// before its own draw — voe_render_frame_draw pushes the object number
	// for every mesh it draws, and voe_render_element's draw pushes the
	// transform for its one draw. There is no ordering in which a shader
	// reads bytes the other left. Widening it further is free until 128,
	// which is the smallest range Vulkan guarantees.
	VkPushConstantRange push = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
			      VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(voe_math_float4x4),
	};
	VkPipelineLayoutCreateInfo layout = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &device->descriptor_layout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push,
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
		.pDepthStencilState = &depth,
		.pColorBlendState = &blend,
		.pDynamicState = &dynamic,
	};
	VkResult result;

	VOE_BASE_DEBUG_ASSERT(device->descriptor_layout != VK_NULL_HANDLE,
			      "building the pipeline before the descriptor layout exists");

	if (voe_render_vk.create_shader_module(device->device, &module_info, NULL,
					       &module) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateShaderModule failed on draw.spv");
		return false;
	}

	stages[0] = (VkPipelineShaderStageCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = module,
		.pName = DRAW_VERTEX_ENTRY,
	};
	stages[1] = (VkPipelineShaderStageCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = module,
		.pName = DRAW_FRAGMENT_ENTRY,
	};

	// ONE LAYOUT FOR BOTH PIPELINES, MADE BY WHICHEVER GETS HERE FIRST. The
	// two describe the same set and the same push constant, and the probe
	// shares it as well — see device_internal.h. A second one would be a
	// second handle for the same description and a leak the day only one of
	// them was destroyed.
	if (device->layout == VK_NULL_HANDLE &&
	    voe_render_vk.create_pipeline_layout(device->device, &layout, NULL,
						 &device->layout) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreatePipelineLayout failed");
		voe_render_vk.destroy_shader_module(device->device, module, NULL);
		return false;
	}
	info.layout = device->layout;

	result = voe_render_vk.create_graphics_pipelines(device->device,
							 VK_NULL_HANDLE, 1, &info,
							 NULL, out);

	// The module is the compiler's input and the pipeline has finished
	// reading it, so it goes away here whether or not the pipeline was made.
	// Keeping it would be keeping a copy of the shader for nobody.
	voe_render_vk.destroy_shader_module(device->device, module, NULL);

	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateGraphicsPipelines failed on the %s pipeline (VkResult %d)",
			       blended ? "blended" : "solid", (int)result);
		*out = VK_NULL_HANDLE;
		return false;
	}
	return true;
}

// The solid one first, because it is the one that makes the layout and the one
// every draw that is not see-through goes through.
bool voe_render_pipelines_create(voe_render_device *device)
{
	return create_pipeline(device, false, &device->pipeline) &&
	       create_pipeline(device, true, &device->pipeline_blended);
}
