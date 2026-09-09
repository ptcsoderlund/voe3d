// Many small rectangles, one draw. The whole element path on the C side: the
// pipeline, the submit that writes one record, the draw that draws every record
// the frame holds, and the one matrix that turns millimetres into clip space.
//
// THE CALLER WRITES A RECORD AND THE CARD BUILDS THE CORNERS. There is no vertex
// buffer and no index buffer on this path and none is bound — the vertex shader
// has its vertex index and its instance index and needs nothing else. What the
// CPU spends per rectangle is one eighty-byte struct assignment instead of four
// voe_render_vertex and six indices, and it triangulates nothing.
//
// AND THE WHOLE FRAME'S WORTH IS ONE DRAW COMMAND. The colour is in the record
// rather than in a shading record, so every rectangle may differ without
// breaking the draw — which is the thing the mesh path cannot do, where one
// draw means one shading record and therefore one colour. See
// voe_render_frame_draw_count, which is how a caller reads that rather than
// believing it.
//
// IT IS NOT A USER INTERFACE PATH AND NOTHING IN HERE IS NAMED AS IF IT WERE. An
// interface is the first caller; debug lines and sprites want the same shape and
// would come through the same two calls.
//
// THE BUFFERS ARE NOT HERE. One mapped record buffer per frame slot is a thing
// the shader reads, so it is built and torn down with the rest of them in
// descriptors.c, and this file only ever writes through the pointer it left.
// What makes writing it safe is the fence at the top of the frame — the same
// fence that makes the object buffer beside it safe, and deliberately not a
// second synchronisation story.
//
// THE PIPELINE SHARES device->layout AND THAT IS THE REASON THE PUSH CONSTANT
// RANGE IS SIXTY-FOUR BYTES. Two pipeline layouts differing in their push
// constant ranges are not compatible, and binding a pipeline with an
// incompatible layout disturbs the descriptor set bindings — which would mean
// rebinding the set after every element draw, in a file that binds it once at
// the top of the frame. One layout is one fewer invariant to hold. See the range
// in device.c and the note in shaders/elements.slang.
//
// IT DIFFERS FROM THE OTHER TWO PIPELINES IN FOUR THINGS AND NOT IN ONE. No
// vertex input state at all, a triangle strip rather than a list, nothing
// culled, and its own shader module. That is why it is not a third `blended`
// argument to create_pipeline in device.c — there would be nothing left of the
// shared description.
//
// BLENDED, DEPTH TEST ON, DEPTH WRITE OFF, exactly as the blended mesh pipeline
// is and for the same two reasons: an element behind an opaque object is still
// hidden by it, and two elements never hide each other, so the order they were
// submitted in is the order they are painted in. The blend is premultiplied
// because the colour target holds premultiplied colour (ADR-0069) and the shader
// does the multiply once at its output.
#include "device_internal.h"

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

// The compiled element shader, in the binary, by the same route draw.spv takes:
// slangc writes it into the build tree and cmake/voe.cmake puts that directory
// on this file's embed path. alignas for the reason device.c gives — the driver
// is handed a const uint32_t * and #embed can only fill bytes.
static alignas(uint32_t) const unsigned char elements_spv[] = {
#embed "elements.spv"
};

// Spelled exactly as shaders/elements.slang spells them; -fvk-use-entrypoint-name
// in cmake/voe.cmake is what keeps these two strings true.
#define ELEMENT_VERTEX_ENTRY "voe_render_element_vertex"
#define ELEMENT_FRAGMENT_ENTRY "voe_render_element_fragment"

// Four corners from a triangle strip: two triangles, and two fewer vertex stage
// invocations per element than the six a triangle list would want. Named because
// it is the count the draw passes and the count the shader's index arithmetic
// assumes, and those are one fact.
#define ELEMENT_VERTICES 4

bool voe_render_element_startup(voe_render_device *device)
{
	VkShaderModuleCreateInfo module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = sizeof(elements_spv),
		.pCode = (const uint32_t *)elements_spv,
	};
	VkShaderModule module = VK_NULL_HANDLE;
	VkPipelineShaderStageCreateInfo stages[2];
	// NOTHING. No bindings and no attributes, because no vertex buffer is
	// bound on this path and the shader reads none — it builds its corner
	// out of SV_VertexID. Vulkan is content with this; a dummy buffer added
	// to quiet something would be a buffer nothing reads.
	VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	};
	// A strip, and each instance is its own strip: primitive assembly
	// restarts per instance, so four vertices per element is two triangles
	// per element and not one long ribbon through all of them.
	VkPipelineInputAssemblyStateCreateInfo assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
	};
	VkPipelineViewportStateCreateInfo viewport = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};
	// NOTHING IS CULLED HERE AND THAT IS THE POINT. A screen-space rectangle
	// has no back face to hide, so the strip's winding is not a fact anybody
	// has to keep right — and with the engine's Y flip in the viewport,
	// culling would make the corner order interact with the flip for no gain
	// at all. frontFace is therefore not stated: with no cull mode there is
	// nothing for it to decide. render/src/probe.c makes the same choice for
	// the same reason.
	VkPipelineRasterizationStateCreateInfo raster = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.lineWidth = 1.0f,
	};
	VkPipelineMultisampleStateCreateInfo multisample = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};
	// The blended pipeline's rules, and GREATER because depth runs backwards
	// in this engine. The test is on so that an element behind an opaque
	// object is hidden by it; the write is off so that two elements do not
	// hide each other, which is what makes submission order paint order.
	VkPipelineDepthStencilStateCreateInfo depth = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_GREATER,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
	};
	// Premultiplied, the same pair the blended mesh pipeline uses: source
	// ONE and destination ONE_MINUS_SRC_ALPHA for colour and alpha both,
	// because the fragment leaves elements.slang with its rgb already
	// multiplied by its own alpha. The textbook SRC_ALPHA pair would
	// multiply a second time and every see-through element would come out
	// too dark.
	VkPipelineColorBlendAttachmentState attachment = {
		.blendEnable = VK_TRUE,
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
	// Dynamic for the reason the other two pipelines are: a resize then
	// rebuilds the targets and the swapchain and no pipeline.
	VkDynamicState dynamic_states[2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo dynamic = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamic_states,
	};
	// Both attachment formats, because dynamic rendering has no render pass
	// to read them from and they have to match what frame.c attaches. The
	// depth one is declared even though nothing is written to it: the
	// rendering block has a depth attachment and a pipeline that denied
	// having one would be invalid rather than merely not writing.
	VkPipelineRenderingCreateInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &device->format.format,
		.depthAttachmentFormat = VOE_RENDER_DEPTH_FORMAT,
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

	VOE_BASE_DEBUG_ASSERT(device != NULL, "starting elements on no device");
	// Shared, not made here, which is what forces this to run after
	// create_pipelines rather than beside it.
	VOE_BASE_ASSERT(device->layout != VK_NULL_HANDLE,
			"building the element pipeline before the layout the other two made");

	if (voe_render_vk.create_shader_module(device->device, &module_info,
					      NULL, &module) != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateShaderModule failed on elements.spv\n");
		return false;
	}

	stages[0] = (VkPipelineShaderStageCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = module,
		.pName = ELEMENT_VERTEX_ENTRY,
	};
	stages[1] = (VkPipelineShaderStageCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = module,
		.pName = ELEMENT_FRAGMENT_ENTRY,
	};
	info.layout = device->layout;

	result = voe_render_vk.create_graphics_pipelines(device->device,
							VK_NULL_HANDLE, 1,
							&info, NULL,
							&device->pipeline_elements);

	// The module was the compiler's input and the pipeline has finished with
	// it, whether or not it was made.
	voe_render_vk.destroy_shader_module(device->device, module, NULL);

	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateGraphicsPipelines failed on the element pipeline (VkResult %d)\n",
			(int)result);
		device->pipeline_elements = VK_NULL_HANDLE;
		return false;
	}
	return true;
}

void voe_render_element_shutdown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "shutting elements down on nothing");

	if (device->device == VK_NULL_HANDLE)
		return;

	if (device->pipeline_elements != VK_NULL_HANDLE) {
		voe_render_vk.destroy_pipeline(device->device,
					       device->pipeline_elements, NULL);
		device->pipeline_elements = VK_NULL_HANDLE;
	}
}

bool voe_render_frame_submit_element(voe_render_device *device,
				     voe_render_element element)
{
	struct voe_render_frame *frame;

	VOE_BASE_ASSERT(device != NULL, "submitting an element to no device");
	VOE_BASE_ASSERT(device->recording,
			"submitting an element with no frame open — voe_render_frame_begin said there was nothing to draw into, or _end has already run");

	// The one refusal, and it is also the answer on a device opened with no
	// element room at all: the count starts at nought and the capacity is
	// nought, so the first submit takes this branch. Returned and not fatal
	// — a capacity chosen too small is a thing a caller can report and act
	// on, the same shape the geometry pools' refusal has.
	if (device->element_count >= device->capacities.elements) {
		fprintf(stderr,
			"render: this frame already holds %u of %u elements; `elements` is too small for what this frame submits\n",
			device->element_count, device->capacities.elements);
		return false;
	}

	frame = voe_render_frame_open(device);

	// Written straight into this slot's own mapped buffer, at this element's
	// number, rather than staged: the buffer is host-visible and the fence
	// at the top of the frame is what says the card has finished reading
	// what was in it two frames ago.
	VOE_BASE_DEBUG_ASSERT(frame->elements_mapped != NULL,
			      "submitting an element to a frame whose element buffer is not mapped");
	memcpy((unsigned char *)frame->elements_mapped +
		       (size_t)device->element_count * sizeof(element),
	       &element, sizeof(element));

	device->element_count++;
	return true;
}

bool voe_render_frame_draw_elements(voe_render_device *device,
				    voe_math_float4x4 transform)
{
	struct voe_render_frame *frame;

	VOE_BASE_ASSERT(device != NULL, "drawing elements on no device");
	VOE_BASE_ASSERT(device->recording,
			"drawing elements with no frame open — voe_render_frame_begin said there was nothing to draw into, or _end has already run");

	if (device->pipeline_elements == VK_NULL_HANDLE) {
		fprintf(stderr,
			"render: this device has no element pipeline, so there is nothing to draw elements with\n");
		return false;
	}

	// Nothing submitted is not a refusal and records no command: a caller
	// that draws an interface with nothing in it this frame is not a caller
	// that has gone wrong, and an instanced draw of nought instances would
	// still be a draw command in the count.
	if (device->element_count == 0)
		return true;

	frame = voe_render_frame_open(device);

	// Unconditional, unlike the mesh draws' bind: there is one element draw
	// in a frame, so a condition would be a comparison that is never false
	// twice. Setting `bound` to this pipeline is what makes the next mesh
	// draw rebind its own — it compares against device->pipeline and
	// device->pipeline_blended, and this is neither.
	//
	// THE VERTEX AND INDEX BUFFER BINDINGS SURVIVE THIS AND SO DOES
	// `bound_transient`. Binding a pipeline does not disturb them, and this
	// pipeline binds nothing of its own, so a mesh draw after an element
	// draw finds the pool pair it left bound. The descriptor set survives it
	// too, because this pipeline shares device->layout — see the header.
	voe_render_vk.cmd_bind_pipeline(frame->commands,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					device->pipeline_elements);
	device->bound = device->pipeline_elements;

	// The surface's transform, into the same range a mesh draw pushes its
	// object number into. Both blocks start at nought and each pipeline
	// pushes its own immediately before its own draw, so neither ever reads
	// the other's bytes.
	voe_render_vk.cmd_push_constants(frame->commands, device->layout,
					 VK_SHADER_STAGE_VERTEX_BIT |
						 VK_SHADER_STAGE_FRAGMENT_BIT,
					 0, sizeof(transform), &transform);

	// THE ONE DRAW, AND THE WHOLE CLAIM OF THIS PATH. Four vertices, one
	// instance per element, no index buffer and no vertex buffer. The
	// shader's instance index is the record's number.
	voe_render_vk.cmd_draw(frame->commands, ELEMENT_VERTICES,
			       device->element_count, 0, 0);
	device->draw_commands++;
	return true;
}

voe_math_float4x4 voe_render_element_transform(voe_math_float2 size)
{
	voe_math_float4x4 m = { 0 };

	// A reciprocal of nothing is what would reach the shader, and a surface
	// with no width is a caller that has not worked out how big it is.
	VOE_BASE_ASSERT(size.x != 0.0f && size.y != 0.0f,
			"asking for the transform of an element surface with no area");

	// Row-major, m[row][column], vectors are columns: this is applied as
	// M · (x, y, 0, 1) and the shader's mul() means the same thing.
	//
	// X: nought maps to -1 and `size.x` to +1, which is left to right.
	m.m[0][0] = 2.0f / size.x;
	m.m[0][3] = -1.0f;

	// Y: NEGATIVE, AND THIS IS THE ONLY NEGATION IN THE ELEMENT PATH. Y
	// runs down in element space — nought is the top — and the engine's one
	// Y flip, the negative viewport height in frame.c, puts +1 in clip space
	// at the *top* of the screen. So nought has to map to +1 and `size.y` to
	// -1, which is a scale of -2/size.y and an offset of +1. There is no
	// second flip anywhere: this row is where element space's convention and
	// the engine's meet, and it was worked out from the viewport rather than
	// arrived at by negating something until the picture looked right.
	m.m[1][1] = -2.0f / size.y;
	m.m[1][3] = 1.0f;

	// Z: THE NEAR PLANE, WHICH IS 1.0 BECAUSE DEPTH RUNS BACKWARDS HERE. The
	// test is GREATER, so an element is in front of anything already drawn.
	// It is a constant and not a function of the input, because the input has
	// no third dimension; the shader hands in a z of nought and the last
	// column is what puts the 1 there.
	m.m[2][3] = 1.0f;

	// W: one, so the division the rasteriser does changes nothing. An
	// element surface is not projected — it is a flat sheet, and a
	// perspective divide by anything else would scale it by a number no
	// caller asked about.
	m.m[3][3] = 1.0f;

	return m;
}
