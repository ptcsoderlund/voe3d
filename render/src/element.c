// Many small rectangles, one draw. The whole element path on the C side: the
// pipeline, the submit that writes one record, the draw that draws a range of
// them, and the two matrices that say what element space is — one onto the
// surface's own plane in metres, and one from there onto the whole target.
//
// THE SECOND MATRIX IS BUILT ON THE FIRST AND THE Y SIGN IS IN THE FIRST ALONE.
// A surface standing in the world and a surface filling the window differ in
// what happens after element space stops, not in what element space is, so
// there is one function that says y runs down and everything else composes onto
// it. Writing the negation twice is the failure this arrangement exists to make
// impossible; see voe_render_element_surface_matrix.
//
// THE CALLER WRITES A RECORD AND THE CARD BUILDS THE CORNERS. There is no vertex
// buffer and no index buffer on this path and none is bound — the vertex shader
// has its vertex index and its instance index and needs nothing else. What the
// CPU spends per rectangle is one eighty-byte struct assignment instead of four
// voe_render_vertex and six indices, and it triangulates nothing.
//
// AND THE WHOLE FRAME'S WORTH IS ONE INSTANCED DRAW COMMAND. The colour is in the
// record rather than in a shading record, so every rectangle may differ without
// breaking the draw — which the two mesh pipelines cannot do: they draw out of the
// pools one thing per draw, and one draw means one shading record and therefore
// one colour. See voe_render_frame_draw_count, which is how a caller reads that
// rather than believing it.
//
// IT IS NOT A USER INTERFACE PATH AND NOTHING IN HERE IS NAMED AS IF IT WERE. An
// interface is the first caller; debug lines and sprites want the same shape and
// would come through the same two calls. A rectangle and a letter are the same
// record and the same draw command: an element that reads its coverage out of a
// distance-field sheet is a glyph, and nothing about it is a text system.
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
// rebinding the set after every element draw, in a file that binds it once as
// a pass opens. One layout is one fewer invariant to hold. See the range
// in device.c and the note in shaders/elements.slang.
//
// IT DIFFERS FROM THE OTHER TWO PIPELINES IN FOUR THINGS AND NOT IN ONE. No vertex
// input state at all, a triangle strip rather than a list, nothing culled, and its
// own shader module. That is why it is not a third `blended` argument to
// create_pipeline in device.c — there would be nothing left of the shared description.
//
// BLENDED, DEPTH TEST ON, DEPTH WRITE OFF, exactly as the blended mesh pipeline
// is and for the same two reasons: an element behind an opaque object is still
// hidden by it, and two elements never hide each other, so the order they were
// submitted in is the order they are painted in. The blend is premultiplied
// because the colour target holds premultiplied colour (ADR-0069) and the shader
// does the multiply once at its output.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <math/float4x4.h>

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

// Metres in a millimetre, which is the whole of ADR-0089 as arithmetic: a GUI
// unit is one millimetre and a millimetre is a thousandth of a metre. It is
// named because it appears in two matrices below and the two have to agree; a
// panel that came out the size of a wall or too small to find is this number
// inverted.
#define METRES_PER_MILLIMETRE 0.001f

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
		VOE_BASE_ERROR("render",
			       "vkCreateShaderModule failed on elements.spv");
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
		VOE_BASE_ERROR("render",
			       "vkCreateGraphicsPipelines failed on the element pipeline (VkResult %d)",
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
		VOE_BASE_ERROR("render",
			       "this frame already holds %u of %u elements; `elements` is too small for what this frame submits",
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

uint32_t voe_render_frame_elements_submitted(const voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL,
			"asking no device how many elements it holds");

	// No assert on `recording`, for the reason voe_render_frame_draw_count
	// gives: the number is worth reading after _end as well as during a
	// frame, and a caller reporting what a frame cost reads it there.
	return device->element_count;
}

// Whether any GLYPH or IMAGE record in the range reads texture slot `texture`.
// A SOLID never reads its sheet_texture and need not have set it, so it names
// nothing whatever the field holds. For the self-sampling assert below and
// nothing else, compiled into a release build only as an operand of sizeof.
//
// IT READS THE RECORDS BACK OUT OF THE MAPPED BUFFER, which the rest of this file
// only ever writes. The buffer is host-visible and coherent, so this is legal
// and merely slow, and a debug build checking the one kind of pass where it
// matters is where slow is the right trade.
[[maybe_unused]] static bool range_names(voe_render_device *device,
					 uint32_t first, uint32_t count,
					 uint32_t texture)
{
	const struct voe_render_frame *frame = voe_render_frame_open(device);
	voe_render_element element;

	for (uint32_t i = first; i < first + count; i++) {
		memcpy(&element,
		       (const unsigned char *)frame->elements_mapped +
			       (size_t)i * sizeof(element),
		       sizeof(element));
		if ((element.kind == VOE_RENDER_ELEMENT_GLYPH ||
		     element.kind == VOE_RENDER_ELEMENT_IMAGE) &&
		    element.sheet_texture == texture)
			return true;
	}
	return false;
}

bool voe_render_frame_draw_elements(voe_render_device *device,
				    voe_math_float4x4 transform, uint32_t first,
				    uint32_t count)
{
	struct voe_render_frame *frame;

	VOE_BASE_ASSERT(device != NULL, "drawing elements on no device");
	// A pass, and not necessarily one with a camera: the transform is the
	// whole of what places an element.
	VOE_BASE_ASSERT(device->pass_open,
			"drawing elements with no pass open — every draw is inside a voe_render_pass_begin and its _pass_end");
	VOE_BASE_ASSERT(!device->pass_shadow,
			"drawing elements in a shadow pass — it has no colour to blend them into");
	VOE_BASE_ASSERT(!device->pass_bounce,
			"drawing elements in a bounce pass — its colour is light, not a picture");

	if (device->pipeline_elements == VK_NULL_HANDLE) {
		VOE_BASE_ERROR("render",
			       "this device has no element pipeline, so there is nothing to draw elements with");
		return false;
	}

	// Written as two comparisons with a subtraction rather than as
	// `first + count > element_count`, which is the same question with an
	// addition in it that can wrap — and a wrapped sum is a range that
	// passes the test and then reads whatever is after the buffer.
	//
	// RETURNED AND NOT ASSERTED, because ordinary staleness arrives here. A
	// caller holding a range from a frame that has gone is a surface nobody
	// rebuilt this frame, and that costs one draw rather than the program.
	if (first > device->element_count ||
	    count > device->element_count - first) {
		VOE_BASE_ERROR("render",
			       "a draw asked for %u elements from %u, and this frame holds %u",
			       count, first, device->element_count);
		return false;
	}

	VOE_BASE_DEBUG_ASSERT(device->pass_target == NULL ||
				      !range_names(device, first, count,
						   device->pass_target->texture),
			      "drawing an element range that shows the target this pass draws into — a picture may not read itself while it is written");

	// Nothing to draw is not a refusal and records no command: a caller that
	// draws an interface with nothing in it this frame is not a caller that
	// has gone wrong, and an instanced draw of nought instances would still
	// be a draw command in the count.
	if (count == 0)
		return true;

	frame = voe_render_frame_open(device);

	// Unconditional, unlike the mesh draws' bind. A frame holds one element
	// draw per surface and a handful of surfaces at most, so tracking this
	// one would save a bind that is already rare — and the mesh draws in
	// between rebind their own anyway, which is what would make the
	// condition false nearly every time it was asked. Setting `bound` to
	// this pipeline is what makes the next mesh
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

	// ONE DRAW FOR THE WHOLE RANGE, AND THE WHOLE CLAIM OF THIS PATH. Four
	// vertices, one instance per element, no index buffer and no vertex
	// buffer.
	//
	// `first` IS THE FIRST INSTANCE, AND THAT IS WHAT MAKES A RANGE COST
	// NOTHING: no second binding, no buffer offset and no push constant —
	// which matters, because this pipeline's sixty-four bytes are the
	// transform and the layout is shared with the mesh pipelines, so there
	// is no room for one.
	//
	// THE SHADER ADDS IT BACK AND HAS TO. Slang gives SV_InstanceID HLSL's
	// meaning — the instance's number *within this draw*, counting from
	// nought however many instances were skipped — so a shader reading it
	// alone would draw the first `count` records of the buffer whatever
	// `first` said, in every range but the first. elements.slang therefore
	// reads SV_StartInstanceLocation beside it and adds the two. That was
	// established by putting the same two rectangles in two ranges and
	// moving only the matrix; see two_ranges_two_matrices_two_draws in
	// tests/elements.c, which is the test that catches it coming back.
	voe_render_vk.cmd_draw(frame->commands, ELEMENT_VERTICES, count, 0,
			       first);
	device->draw_commands++;
	return true;
}

voe_math_float4x4 voe_render_element_surface_matrix(voe_math_float2 size)
{
	voe_math_float4x4 m = { 0 };

	// A surface with no width is a caller that has not worked out how big it
	// is, and the centring below would put its middle nowhere.
	VOE_BASE_ASSERT(size.x != 0.0f && size.y != 0.0f,
			"asking for the plane of an element surface with no area");

	// Row-major, m[row][column], vectors are columns: this is applied as
	// M · (x, y, 0, 1) and the shader's mul() means the same thing.
	//
	// X: millimetres to metres, and the surface's middle to the origin. Its
	// left edge lands at minus half its width in metres and its right edge
	// at plus half.
	m.m[0][0] = METRES_PER_MILLIMETRE;
	m.m[0][3] = -0.5f * size.x * METRES_PER_MILLIMETRE;

	// Y: NEGATIVE, AND THIS IS THE ONLY NEGATION IN THE ELEMENT PATH. Y runs
	// down in element space — nought is the top edge — and the world it is
	// being placed into runs +Y up. So the top edge has to land at plus half
	// the height and the bottom edge at minus half, which is this scale and
	// this offset. There is no second flip anywhere: this row is where
	// element space's convention and the engine's meet, and everything else
	// on this path composes onto it rather than deciding again.
	m.m[1][1] = -METRES_PER_MILLIMETRE;
	m.m[1][3] = 0.5f * size.y * METRES_PER_MILLIMETRE;

	// Z: the same scale, though nothing drawn here has a third dimension —
	// the shader hands in a z of nought. It is the millimetre factor rather
	// than a nought because a matrix with a nought on its diagonal is one
	// that cannot be inverted, and a caller composing this into a chain has
	// every right to expect a scale rather than a projection onto a plane.
	m.m[2][2] = METRES_PER_MILLIMETRE;

	// W: one. A surface is a flat sheet and is not projected by anything in
	// here; what projects it, if anything does, is the caller's camera.
	m.m[3][3] = 1.0f;

	return m;
}

voe_math_float4x4 voe_render_element_transform(voe_math_float2 size)
{
	// The surface's own metres onto the whole target: half its width in
	// metres to one, half its height to one, and just inside the near plane
	// for z.
	voe_math_float4x4 onto_the_target = { 0 };

	// A reciprocal of nothing is what would reach the shader, and the
	// surface matrix below asserts on the same thing for its own reason.
	VOE_BASE_ASSERT(size.x != 0.0f && size.y != 0.0f,
			"asking for the transform of an element surface with no area");

	// BOTH SCALES ARE POSITIVE AND THAT IS THE POINT. By the time a point
	// reaches this matrix it has already been through
	// voe_render_element_surface_matrix, which is where Y turned round; the
	// engine's one Y flip in the viewport puts +1 in clip space at the top of
	// the screen, and the surface's top edge is already at +y in metres. A
	// minus sign here as well is the double flip that looks correct until
	// something is culled.
	onto_the_target.m[0][0] = 2.0f / (size.x * METRES_PER_MILLIMETRE);
	onto_the_target.m[1][1] = 2.0f / (size.y * METRES_PER_MILLIMETRE);

	// Z: JUST INSIDE THE NEAR PLANE, AND DELIBERATELY NOT ON IT. Depth runs
	// backwards here, so 1.0 is the near plane and a smaller number is
	// further from the eye; the test is GREATER, so this still passes in
	// front of everything the camera can see. It is a constant and not a
	// function of the input, because a surface filling the target has no
	// depth of its own to keep.
	//
	// IT IS NOT 1.0, AND PUTTING IT BACK IS BUG 001 AGAIN. At exactly 1.0
	// every vertex of this surface leaves here with z bit-exactly equal to
	// w, standing on the near clip boundary. Vulkan's view volume is
	// 0 <= z <= w *inclusive*, so a conformant implementation keeps it — but
	// it is the one position in a whole continuum of valid ones where two
	// implementations may legitimately disagree, and one of them threw the
	// whole surface away: nothing mapped onto the window reached the screen,
	// while the same records drew perfectly through the same pipeline on
	// another driver. Standing on the boundary buys nothing; ADR-0111 moved
	// it off, and tests/elements.c asserts z < w strictly so that this
	// cannot be tidied back.
	//
	// AND IT IS 0.9999 RATHER THAN A ROUNDER RETREAT, BECAUSE WHAT STANDING
	// BACK COSTS IS BEING OCCLUDABLE. This depth is fixed, so how much room
	// it leaves in front of itself is the caller's camera's business:
	// reverse-Z with a finite far plane gives d = (n/(f - n)) * (f/z - 1),
	// which inverts to z = f / (1 + d * (f - n) / n) — the world distance at
	// which geometry starts drawing over the overlay. At the dev camera's
	// n = 0.1 m, f = 100 m:
	//
	//     0.9999 -> 0.100010 m, a shell 0.01 mm deep
	//     0.999  -> 0.1001 m,   a shell 0.1 mm deep
	//     0.9    -> 0.1111 m,   a shell 11 mm deep
	//
	// That distance is where the shell of world in front of the overlay
	// begins, and it is the whole cost of the retreat. A shell a hundredth
	// of a millimetre deep, right at the front of the view volume, is not
	// somewhere an object ends up by accident; eleven millimetres is a gap
	// an ordinary object walks into, and an interface disappearing behind
	// the scenery is the same complaint as bug 001 with a different cause.
	// So: off the boundary by as little as still says something in a float,
	// and no further.
	onto_the_target.m[2][3] = 0.9999f;

	// W: one, so the division the rasteriser does changes nothing. A surface
	// filling the target is not projected — it is a flat sheet, and a
	// perspective divide by anything else would scale it by a number no
	// caller asked about.
	onto_the_target.m[3][3] = 1.0f;

	return voe_math_float4x4_mul(onto_the_target,
				     voe_render_element_surface_matrix(size));
}

voe_math_float2 voe_render_element_surface_size(voe_platform_size target,
						float pixels_per_millimetre)
{
	voe_math_float2 size;

	// Dividing by it is what happens next, and an infinity reaching a matrix
	// reaches a shader as a blank window rather than as anything that says
	// what went wrong. Negative is refused with it: a surface cannot be a
	// negative number of millimetres across, and the caller that computed one
	// has a sign error worth stopping for.
	VOE_BASE_ASSERT(pixels_per_millimetre > 0.0f,
			"asking how many millimetres a target holds at nought or fewer pixels per millimetre");

	// Both axes by the one number, which is the whole of what this call is
	// for: a wider window holds more millimetres, it does not hold wider
	// millimetres.
	size.x = (float)target.width / pixels_per_millimetre;
	size.y = (float)target.height / pixels_per_millimetre;

	return size;
}
