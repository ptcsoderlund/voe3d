// TWO CLAIMS ABOUT MATRICES, AND ONLY ONE OF THEM NEEDS A GRAPHICS CARD.
//
// The first is that this engine's depth really does run backwards, and it is
// checked on the CPU: voe_render_cube_projection is arithmetic, so a test can
// read the matrix it returns and multiply points through it without a driver
// anywhere. Near plane at 1.0, far plane approached at 0.0, nearer means
// greater, and no negated Y. That half runs on every machine, build box
// included, and it is the half that catches somebody "correcting" the
// projection to the shape every tutorial writes.
//
// The second is that slangc really was invoked with -matrix-layout-row-major,
// and that one cannot be checked without running a shader. A known matrix goes
// into the engine's own uniform buffer through the engine's own descriptor, and
// matrix_probe.slang reports three of its elements back as colour. With the flag
// the shader reads the elements C wrote at [0][3], [1][3] and [2][3]; without it
// it reads the ones C wrote at [3][0], [3][1] and [3][2], because the same
// sixteen floats mean something else. The matrix below is filled so that all six
// of those are different numbers, far apart, so the answer names the layout
// rather than merely differing from it.
//
// WHY THE FLAG IS WORTH A TEST AT ALL. Remove it and nothing fails to compile,
// nothing warns, and every transform in the engine comes out transposed. That is
// the whole class of bug this file exists for, and CLAUDE.md and float4x4.h both
// say so in words — this is the same statement in a form that runs.
//
// IT INCLUDES render's INTERNAL HEADER BY RELATIVE PATH, exactly as
// tests/offscreen.c and tests/loader.c do and for the same reason: a headless
// device, the probe and the frame slots are not render's public surface and must
// not become part of it so that a test can see them.
//
// A MACHINE WITH NO USABLE VULKAN STILL RUNS THE FIRST HALF AND SKIPS THE
// SECOND, AND SAYS SO. There is no driver on a headless build box and that is
// not a broken checkout.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

// Small and square: every pixel of it is copied to the CPU, and the probe covers
// all of them, so one sample is as good as all of them. Square so that a
// mistake swapping width for height cannot hide.
#define SIDE 16
#define IMAGE_BYTES ((VkDeviceSize)SIDE * SIDE * 4)

// The channel order of VK_FORMAT_B8G8R8A8_UNORM, which is the format a headless
// device takes and which this test checks it really got before reading a byte.
#define BLUE 0
#define GREEN 1
#define RED 2

// Element (row, column) of the probe matrix is (row * 4 + column) / 16, so every
// one of the sixteen is a different value and flat memory float k holds k / 16.
// That makes the two layouts' answers arithmetic rather than a lookup:
//
//   row-major     m[0][3] m[1][3] m[2][3] are floats 3, 7, 11
//   column-major  the same expressions are floats 12, 13, 14
//
// Both sets are inside 0..1 so they survive an unorm colour, and the two are a
// hundred byte values apart so no rounding mode can turn one into the other.
#define PROBE_ELEMENT(row, column) ((float)((row) * 4 + (column)) / 16.0f)

// What the three channels must read, as bytes, if the flag was in force. 255 *
// 3/16, 7/16 and 11/16.
#define EXPECTED_RED 48
#define EXPECTED_GREEN 112
#define EXPECTED_BLUE 175

// What they would read without it — never asserted as a pass, only used to say
// so in the failure message, because "48 but got 191" is a diagnosis and "48 but
// got something else" is a puzzle.
#define COLUMN_MAJOR_RED 191

// A byte or two of slack. The float-to-unorm conversion is round-to-nearest but
// the exact tie-breaking is the implementation's, and the two layouts differ by
// far more than this.
#define TOLERANCE 3

// ------------------------------------------------ the half with no graphics card

// The depth the rasteriser would see for a point this far in front of the
// camera, which is clip z over clip w. Distance is positive and the camera looks
// along its own -Z, so the point is at view-space z = -distance.
static float depth_at(voe_math_float4x4 projection, float distance)
{
	voe_math_float4 point = { 0.0f, 0.0f, -distance, 1.0f };
	voe_math_float4 clip = voe_math_float4x4_mul_float4(projection, point);

	VOE_TEST_CHECK(clip.w != 0.0f);
	if (clip.w == 0.0f)
		return 0.0f;
	return clip.z / clip.w;
}

static void check_projection(void)
{
	VkExtent2D extent = { 800, 600 };
	voe_math_float4x4 projection = voe_render_cube_projection(extent);
	// The distance at which depth is exactly 1, which is the near plane.
	// Read out of the matrix rather than assumed, because the near plane is
	// cube.c's constant and not this file's business — what is under test is
	// the shape of the matrix, not the number in it.
	float near_plane = projection.m[2][3];

	// w = -z, which is what makes the perspective divide a divide by
	// distance. Without this the two checks below say nothing.
	VOE_TEST_CHECK_FLOAT(projection.m[3][2], -1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(projection.m[3][3], 0.0f, 0.0f);

	// THE FAR PLANE IS AT INFINITY, AND THIS IS ITS SIGNATURE. clip.z is the
	// near plane for every vertex, with no term in z at all, so depth is
	// near / distance and reaches 0 only in the limit. A finite far plane
	// would put a non-zero number here.
	VOE_TEST_CHECK_FLOAT(projection.m[2][2], 0.0f, 0.0f);
	VOE_TEST_CHECK(near_plane > 0.0f);

	// NOTHING IN THE PROJECTION NEGATES Y. The engine's one Y flip is the
	// negative viewport height in frame.c; a minus sign here as well would
	// flip twice, which is invisible until something is culled. Both scales
	// are positive.
	VOE_TEST_CHECK(projection.m[0][0] > 0.0f);
	VOE_TEST_CHECK(projection.m[1][1] > 0.0f);

	// A wider target than it is tall divides x by the aspect ratio, so the
	// horizontal scale is the smaller of the two. This is what catches an
	// aspect correction applied to the wrong axis, which stretches the
	// picture without ever failing.
	VOE_TEST_CHECK(projection.m[0][0] < projection.m[1][1]);

	// DEPTH RUNS BACKWARDS: the near plane is 1, not 0.
	VOE_TEST_CHECK_FLOAT(depth_at(projection, near_plane), 1.0f, 0.0001f);

	// And nearer is greater, all the way out. Three distances rather than
	// two, because two of them could pass on a matrix that is merely
	// constant.
	VOE_TEST_CHECK(depth_at(projection, near_plane * 2.0f) <
		       depth_at(projection, near_plane));
	VOE_TEST_CHECK(depth_at(projection, near_plane * 100.0f) <
		       depth_at(projection, near_plane * 2.0f));

	// The far plane is approached and never reached, and it is 0 rather than
	// 1. A long way out is nearly zero and still positive.
	VOE_TEST_CHECK(depth_at(projection, near_plane * 100000.0f) > 0.0f);
	VOE_TEST_CHECK(depth_at(projection, near_plane * 100000.0f) < 0.001f);

	// Everything in front of the near plane is inside 0..1, which is the
	// range a depth buffer stores and the range the viewport maps onto.
	VOE_TEST_CHECK(depth_at(projection, near_plane * 3.0f) > 0.0f);
	VOE_TEST_CHECK(depth_at(projection, near_plane * 3.0f) < 1.0f);
}

// --------------------------------------------------------- the half that draws

// The one function this test needs that render's own code never calls. Reading
// an image back into memory is not something the engine does, so it is resolved
// here by hand rather than added to the table — the same reasoning, and the same
// shape, as render/tests/offscreen.c.
static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

static bool resolve_readback(voe_render_device *device)
{
	copy_image_to_buffer = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(device->device,
						  "vkCmdCopyImageToBuffer");

	// Core Vulkan 1.0, so a driver without it is broken rather than merely
	// old — but a test that called through a null pointer would die where it
	// is supposed to report, and the analyser is right to say so.
	return copy_image_to_buffer != NULL;
}

// The matrix under test, written straight into the slot's mapped uniform buffer.
// The view and projection members are left as they are: the probe reads model
// and nothing else, and filling them would suggest they mattered.
static void write_probe_matrix(const struct voe_render_frame *frame)
{
	struct voe_render_uniforms *uniforms = frame->uniforms_mapped;

	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++)
			uniforms->model.m[row][column] =
				PROBE_ELEMENT(row, column);
	}
}

static void record_probe(voe_render_device *device,
			 const struct voe_render_frame *frame,
			 VkPipeline pipeline, VkBuffer buffer)
{
	VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VkBufferImageCopy region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.imageExtent = { SIDE, SIDE, 1 },
	};
	// What makes the copy visible to a map below. The fence this submit
	// signals is documented to do the same job, and saying it here as well
	// costs one barrier and removes the question.
	VkMemoryBarrier2 visible = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
		.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &visible,
	};

	voe_render_vk.begin_command_buffer(frame->commands, &begin);

	// The probe leaves the colour target in TRANSFER_SRC_OPTIMAL, which is
	// exactly the layout this copy wants.
	voe_render_probe_draw(device, frame, pipeline,
			      voe_render_frame_viewport(device->resolution));

	copy_image_to_buffer(frame->commands, frame->target.colour.image,
			     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1,
			     &region);
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);

	voe_render_vk.end_command_buffer(frame->commands);
}

// The middle of the image. The probe covers every pixel, so this is a sample and
// not a search.
static void check_reported_matrix(const unsigned char *pixels)
{
	const unsigned char *pixel =
		pixels + ((size_t)(SIDE / 2) * SIDE + SIDE / 2) * 4;

	if (pixel[RED] > COLUMN_MAJOR_RED - TOLERANCE &&
	    pixel[RED] < COLUMN_MAJOR_RED + TOLERANCE) {
		// Not a second assertion, a diagnosis: this is the exact value
		// the other layout produces, so the flag is the thing to look
		// at and not the shader.
		printf("the shader read this matrix column-major: slangc was invoked without -matrix-layout-row-major\n");
	}

	// Through the float check because voe::testing has no near-integer one
	// and adding it would be editing another folder. The values are byte
	// counts and the tolerance is in bytes; the message reads correctly
	// either way, which is what that macro is for.
	VOE_TEST_CHECK_FLOAT((float)pixel[RED], (float)EXPECTED_RED,
			     (float)TOLERANCE);
	VOE_TEST_CHECK_FLOAT((float)pixel[GREEN], (float)EXPECTED_GREEN,
			     (float)TOLERANCE);
	VOE_TEST_CHECK_FLOAT((float)pixel[BLUE], (float)EXPECTED_BLUE,
			     (float)TOLERANCE);
}

int main(void)
{
	voe_base_arena *arena;
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_frame *frame;
	struct voe_render_buffer readback = { 0 };
	VkPipeline pipeline;
	VkCommandBufferSubmitInfo commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	};
	VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &commands,
	};
	// vkMapMemory hands back its pointer through a void **, which is
	// Vulkan's signature and not one this engine gets to choose.
	void *mapped = NULL;

	// First, and with no device at all: the projection is arithmetic.
	check_projection();

	arena = voe_base_arena_new(64 * 1024);
	device = voe_render_device_new_headless(arena, size, &error);
	if (device == NULL) {
		// No Vulkan on the machine, or no card that meets what the
		// engine requires. Both are the build box and neither is this
		// engine being wrong, so they are a skip — and the projection
		// half above has already run and counted.
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: the shader half needs a driver: %s\n",
			       voe_base_error_string(error));
			voe_base_arena_destroy(arena);
			return voe_test_result();
		}
		VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// Everything below reads bytes in this order. A headless device asks no
	// surface and takes this format outright, so this is a claim about
	// device.c and not about the machine.
	VOE_TEST_CHECK_INT(device->format.format, VK_FORMAT_B8G8R8A8_UNORM);
	VOE_TEST_CHECK_INT(device->resolution.width, SIDE);

	frame = &device->frames[0];

	if (!resolve_readback(device)) {
		VOE_TEST_CHECK(false);
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// render's own buffer helper, host-visible and coherent so that reading
	// it after the fence needs no invalidate.
	VOE_TEST_CHECK(voe_render_buffer_build(
		device, &readback, IMAGE_BYTES, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	pipeline = voe_render_probe_pipeline_new(device);
	VOE_TEST_CHECK(pipeline != VK_NULL_HANDLE);

	if (readback.buffer != VK_NULL_HANDLE && pipeline != VK_NULL_HANDLE) {
		write_probe_matrix(frame);
		record_probe(device, frame, pipeline, readback.buffer);

		commands.commandBuffer = frame->commands;

		// The slot's own fence, reset first because device.c makes them
		// signalled so that a first frame has something to wait on.
		voe_render_vk.reset_fences(device->device, 1,
					   &frame->submitted);
		VOE_TEST_CHECK_INT(voe_render_vk.queue_submit2(device->queue, 1,
							       &submit,
							       frame->submitted),
				   VK_SUCCESS);
		VOE_TEST_CHECK_INT(voe_render_vk.wait_for_fences(device->device,
								 1,
								 &frame->submitted,
								 VK_TRUE,
								 UINT64_MAX),
				   VK_SUCCESS);

		VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
							    readback.memory, 0,
							    VK_WHOLE_SIZE, 0,
							    &mapped),
				   VK_SUCCESS);
		if (mapped != NULL) {
			check_reported_matrix(mapped);
			voe_render_vk.unmap_memory(device->device,
						   readback.memory);
		} else {
			VOE_TEST_CHECK(mapped != NULL);
		}
	}

	// Idle before anything the GPU touched goes away. The device's own
	// teardown does this for what it owns; these two are this file's.
	voe_render_vk.device_wait_idle(device->device);
	if (pipeline != VK_NULL_HANDLE)
		voe_render_vk.destroy_pipeline(device->device, pipeline, NULL);
	voe_render_buffer_teardown(device, &readback);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
