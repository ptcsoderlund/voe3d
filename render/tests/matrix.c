// ONE CLAIM, AND IT CANNOT BE CHECKED WITHOUT RUNNING A SHADER: that slangc
// really was invoked with -matrix-layout-row-major.
//
// A known matrix goes into the engine's own uniform buffer through the engine's
// own descriptor, and matrix_probe.slang reports three of its elements back as
// colour. With the flag the shader reads the elements C wrote at [0][3], [1][3]
// and [2][3]; without it it reads the ones C wrote at [3][0], [3][1] and [3][2],
// because the same sixteen floats mean something else. The matrix below is
// filled so that all six of those are different numbers, far apart, so the
// answer names the layout rather than merely differing from it.
//
// WHY THE FLAG IS WORTH A TEST AT ALL. Remove it and nothing fails to compile,
// nothing warns, and every transform in the engine comes out transposed. That is
// the whole class of bug this file exists for, and CLAUDE.md and float4x4.h both
// say so in words — this is the same statement in a form that runs.
//
// IT USED TO HOLD TWO MORE CLAIMS AND CARD 018 MOVED BOTH. The reversed-depth
// projection is 3d/tests/projection.c's, because the projection matrix is built
// in `3d` now; the camera's orbit and the two cubes are gone with the cube, and
// what is left of that camera is checked in scene/tests/camera.c. Neither of
// them needed a graphics card, which is why neither of them belongs in a file
// that skips without one.
//
// IT INCLUDES render's INTERNAL HEADER BY RELATIVE PATH, exactly as
// tests/offscreen.c and tests/loader.c do and for the same reason: the probe and
// the frame slots are not render's public surface and must not become part of it
// so that a test can see them.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO. There is no driver on a
// headless build box and that is not a broken checkout.
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

// The channel order of VK_FORMAT_B8G8R8A8_SRGB, which is the format a headless
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
// Both sets are inside 0..1 so they survive a colour, and the two are far enough
// apart in bytes that no rounding mode can turn one into the other.
#define PROBE_ELEMENT(row, column) ((float)((row) * 4 + (column)) / 16.0f)

// What the three channels must read, as bytes, if the flag was in force.
//
// THEY ARE NOT 255 TIMES THE ELEMENT ANY MORE, AND CARD 019 IS WHY. The target
// is an sRGB format now, so the hardware applies the sRGB curve to whatever the
// shader wrote as it stores it: 3/16 is written as 0.1875 and stored as 120
// rather than 48. The numbers below are that encoding of 3/16, 7/16 and 11/16,
// and the claim this file makes is untouched — which layout was in force is
// still the only thing these three bytes can be read as.
#define EXPECTED_RED 120
#define EXPECTED_GREEN 177
#define EXPECTED_BLUE 216

// What they would read without it — never asserted as a pass, only used to say
// so in the failure message, because "120 but got 225" is a diagnosis and "120
// but got something else" is a puzzle. The encoding of 12/16.
#define COLUMN_MAJOR_RED 225

// A few bytes of slack. The conversion to an sRGB byte is specified but its last
// bit is the implementation's, and the two layouts differ here by twenty-four
// bytes at the closest of the three channels.
#define TOLERANCE 4

// The probe draws three vertices out of its own shader and uploads nothing, so
// every capacity here is the smallest a device will accept. A device is not
// allowed to be made with room for nothing, which is what makes these ones
// rather than noughts.
static const voe_render_capacities PROBE_CAPACITIES = {
	.vertices = 1,
	.indices = 1,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.passes = 1,
};

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
// The projection member is left as it is: the probe reads view and nothing else,
// and filling the other would suggest it mattered.
static void write_probe_matrix(const struct voe_render_frame *frame)
{
	voe_render_view *uniforms = frame->uniforms_mapped;

	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++)
			uniforms->view.m[row][column] =
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

	arena = voe_base_arena_new(64 * 1024);
	device = voe_render_device_new_headless(arena, size, PROBE_CAPACITIES,
					       &error);
	if (device == NULL) {
		// No Vulkan on the machine, or no card that meets what the
		// engine requires. Both are the build box and neither is this
		// engine being wrong, so they are a skip.
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
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
	VOE_TEST_CHECK_INT(device->format.format, VK_FORMAT_B8G8R8A8_SRGB);
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
