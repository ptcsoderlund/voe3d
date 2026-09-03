// BACK FACES ARE CULLED, AND THE Y FLIP, THE WINDING AND THE FRONT-FACE CONSTANT
// AGREE ABOUT WHICH WAY ROUND THAT IS. Three separate lines in three files say
// one thing between them — the negative viewport height in
// voe_render_frame_viewport, the counter-clockwise faces in cube.c, and
// VK_FRONT_FACE_COUNTER_CLOCKWISE beside VK_CULL_MODE_BACK_BIT in device.c — and
// none of them means anything alone. CLAUDE.md says flipping twice looks exactly
// like flipping none until something is culled; this is the test that notices.
//
// HOW A BACK FACE GETS IN FRONT OF THE RASTERISER WITHOUT A SECOND SHADER. The
// viewport is dynamic state, so the same cube drawn through the mirror of the
// engine's viewport has every face wound the other way round in framebuffer
// space: every face the engine would draw is culled and every face it would cull
// is drawn. That leaves the pipeline — the thing actually under test — untouched,
// and it means the front-facing half of this test runs the engine's own viewport
// function rather than a copy of it.
//
// THE CLAIM IS THE BLUE CHANNEL AT THE CENTRE, AND THAT IS NOT AN ARBITRARY
// SAMPLE. The camera looks at the origin, so the centre pixel is where the ray
// through the middle of the cube lands. Going in, that ray enters through the +Z
// face; going out, it leaves through the -Z face. Every one of the +Z face's four
// corners is a colour with blue at 1, and every one of the -Z face's four corners
// is a colour with blue at 0 — see the corner table in cube.c — so whatever the
// interpolation does, the centre pixel reads blue 255 when the engine drew the
// faces pointing at us and blue 0 when it drew the ones pointing away. One
// channel, two values, and no tolerance needed.
//
// IT USED TO BE A TRIANGLE AND THE CLAIM USED TO BE "NOTHING WAS DRAWN". A
// single triangle mirrored is culled entirely, so the old test could ask for an
// image identical to the clear. A cube mirrored is not empty — it is the same
// cube seen from the inside — so that claim could not survive the geometry
// changing and was replaced by the one above rather than weakened.
//
// AND THE DEPTH TEST IS CHECKED BY THE CUBE BEING THERE AT ALL. Depth runs
// backwards here: cleared to 0, compared GREATER. Both of the ways to get that
// wrong reject every fragment rather than sorting them wrongly — LESS against a
// clear of 0, or GREATER against a clear of 1 — so an inverted depth test does
// not draw a confusing picture, it draws nothing. drawn_pixels() below is what
// says so, and it is the automated half of the card's "flip the comparison on
// purpose and confirm it looks wrong".
//
// IT RUNS HEADLESS AND THAT IS WHY IT CAN BE A TEST AT ALL. Drawing into an
// offscreen image needs no window, no compositor and no surface, so this runs
// under ctest on a machine with nothing but a driver — see
// voe_render_device_new_headless in device_internal.h.
//
// It includes render's internal header directly, by relative path, exactly as
// tests/loader.c does and for the same reason: a headless device and the
// engine's own draw are not render's public surface and must not become part of
// it so that a test can see them.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO. There is no driver on a
// headless build box and that is not a broken checkout. The skip is a pass and
// it prints its reason.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

// Square, so that a mistake that swaps width for height cannot hide, and small,
// because every pixel of it is copied to the CPU and read. Every sample point
// below is in these coordinates and moving this number moves all of them.
#define SIDE 64
#define IMAGE_BYTES ((VkDeviceSize)SIDE * SIDE * 4)

// The two images land one after the other in one buffer. Front first, because
// the clear colour every other check is measured against is read out of it.
#define FRONT_OFFSET ((VkDeviceSize)0)
#define BACK_OFFSET IMAGE_BYTES

// The channel order of VK_FORMAT_B8G8R8A8_UNORM, which is the format a headless
// device takes and which the test checks it really got before reading a byte.
#define BLUE 0
#define GREEN 1
#define RED 2

// The one function this test needs that render's own code never calls. Reading
// an image back into memory is not something the engine does, so it is resolved
// here by hand rather than added to the table — putting it there would be adding
// surface for a test, which rule 10 is there to stop. Everything else this file
// touches, buffers included, the engine already does for itself.
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

// The mirror of voe_render_frame_viewport: y at the top and a positive height,
// which is what the engine would have if it did not flip. Written out rather
// than derived, so that it stays the mirror even if the real one changes shape.
static VkViewport mirrored_viewport(VkExtent2D extent)
{
	VkViewport viewport = {
		.y = 0.0f,
		.width = (float)extent.width,
		.height = (float)extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	return viewport;
}

// One slot draws one case into its own target and copies it into the buffer.
// Two slots and not one, because a second draw into the same image would have to
// wait for the first copy to finish reading it — a dependency this test would
// then own and get wrong. Two targets already exist; using both costs nothing,
// and each slot has its own depth image as well, which is the other half of why
// one slot could not serve both cases.
static void record_case(voe_render_device *device,
			const struct voe_render_frame *frame,
			VkViewport viewport, VkBuffer buffer,
			VkDeviceSize offset)
{
	VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VkBufferImageCopy region = {
		.bufferOffset = offset,
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

	// The engine's own drawing, matrices and all, which leaves the colour
	// target in TRANSFER_SRC_OPTIMAL — which is exactly the layout this copy
	// wants.
	voe_render_frame_draw(device, frame, viewport);

	copy_image_to_buffer(frame->commands, frame->target.colour.image,
			     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1,
			     &region);
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);

	voe_render_vk.end_command_buffer(frame->commands);
}

static const unsigned char *pixel_at(const unsigned char *image, uint32_t x,
				     uint32_t y)
{
	return image + ((size_t)y * SIDE + (size_t)x) * 4;
}

static bool same_colour(const unsigned char *a, const unsigned char *b)
{
	return memcmp(a, b, 4) == 0;
}

// How many pixels of an image are something other than the colour it was cleared
// to. What it is really measuring is that anything was drawn at all, which is
// what an inverted depth test takes away — see the note at the top.
static int drawn_pixels(const unsigned char *image, const unsigned char *clear)
{
	int count = 0;

	for (uint32_t y = 0; y < SIDE; y++) {
		for (uint32_t x = 0; x < SIDE; x++) {
			if (!same_colour(pixel_at(image, x, y), clear))
				count++;
		}
	}
	return count;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	VkCommandBufferSubmitInfo commands[2];
	VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 2,
		.pCommandBufferInfos = commands,
	};
	// vkMapMemory hands back its pointer through a void **, which is
	// Vulkan's signature and not one this engine gets to choose. Nothing
	// else here takes an address of an address.
	void *mapped = NULL;
	const unsigned char *pixels;
	const unsigned char *clear;
	const unsigned char *front_centre;
	const unsigned char *back_centre;

	// Two slots, two targets, two command buffers, submitted together. A
	// build with one frame in flight has nowhere to put the second case.
	VOE_TEST_CHECK(VOE_RENDER_FRAMES_IN_FLIGHT >= 2);
	if (VOE_RENDER_FRAMES_IN_FLIGHT < 2) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	device = voe_render_device_new_headless(arena, size, &error);
	if (device == NULL) {
		// No Vulkan on the machine, or no card that meets what the
		// engine requires. Both are the build box and neither is this
		// engine being wrong, so they are a skip. Anything else is a
		// driver that refused something, and that is a failure.
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
	VOE_TEST_CHECK_INT(device->format.format, VK_FORMAT_B8G8R8A8_UNORM);
	VOE_TEST_CHECK_INT(device->resolution.width, SIDE);
	VOE_TEST_CHECK_INT(device->resolution.height, SIDE);

	if (!resolve_readback(device)) {
		VOE_TEST_CHECK(false);
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// render's own buffer helper. Coherent as well as visible, so that
	// reading it after the fence needs no invalidate call.
	VOE_TEST_CHECK(voe_render_buffer_build(
		device, &readback, IMAGE_BYTES * 2,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	if (readback.buffer == VK_NULL_HANDLE) {
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	record_case(device, &device->frames[0],
		    voe_render_frame_viewport(device->resolution),
		    readback.buffer, FRONT_OFFSET);
	record_case(device, &device->frames[1],
		    mirrored_viewport(device->resolution), readback.buffer,
		    BACK_OFFSET);

	commands[0] = (VkCommandBufferSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = device->frames[0].commands,
	};
	commands[1] = (VkCommandBufferSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = device->frames[1].commands,
	};

	// The slot's own fence, reset first because device.c makes them
	// signalled so that a first frame has something to wait on.
	voe_render_vk.reset_fences(device->device, 1,
				   &device->frames[0].submitted);
	VOE_TEST_CHECK_INT(voe_render_vk.queue_submit2(device->queue, 1, &submit,
						       device->frames[0].submitted),
			   VK_SUCCESS);
	VOE_TEST_CHECK_INT(voe_render_vk.wait_for_fences(device->device, 1,
							 &device->frames[0].submitted,
							 VK_TRUE, UINT64_MAX),
			   VK_SUCCESS);

	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		pixels = mapped;

		// A corner of the front image, which is outside the cube
		// whichever way up it is — the cube spans about two fifths of
		// the image. Taking the clear colour from the picture rather
		// than from a constant means frame.c is free to change it
		// without touching this file.
		clear = pixel_at(pixels + FRONT_OFFSET, 1, 1);

		front_centre = pixel_at(pixels + FRONT_OFFSET, SIDE / 2,
					SIDE / 2);
		back_centre = pixel_at(pixels + BACK_OFFSET, SIDE / 2, SIDE / 2);

		// Both cases drew the cube over the centre. If either of these
		// is the clear colour then nothing was drawn there and every
		// claim below would pass or fail for the wrong reason.
		VOE_TEST_CHECK(!same_colour(front_centre, clear));
		VOE_TEST_CHECK(!same_colour(back_centre, clear));

		// THE WHOLE CLAIM, IN ONE CHANNEL. Drawn the engine's way, the
		// centre of the image is on the +Z face, every corner of which
		// has blue at 1. Drawn through the mirrored viewport, the faces
		// pointing at us are culled instead and the centre is on the -Z
		// face, every corner of which has blue at 0.
		VOE_TEST_CHECK_INT(front_centre[BLUE], 255);
		VOE_TEST_CHECK_INT(back_centre[BLUE], 0);

		// The +Z face's corners run from blue through magenta and white
		// to cyan, so red and green are somewhere in between and blue
		// is the largest of the three. This is what pins the near face
		// as the one that was drawn rather than merely something blue.
		VOE_TEST_CHECK(front_centre[BLUE] > front_centre[RED]);
		VOE_TEST_CHECK(front_centre[BLUE] > front_centre[GREEN]);

		// Anything at all was drawn — which is what an inverted depth
		// comparison takes away, because a clear of 0 with GREATER
		// admits every fragment and either mistake admits none. See the
		// note at the top of this file.
		VOE_TEST_CHECK(drawn_pixels(pixels + FRONT_OFFSET, clear) > 0);
		VOE_TEST_CHECK(drawn_pixels(pixels + BACK_OFFSET, clear) > 0);

		// And it is a cube and not the whole image: back-face culling
		// leaves the clear colour everywhere outside the silhouette. A
		// pipeline that drew every face regardless would still leave
		// the corners clear, so this is a sanity bound rather than the
		// culling claim — that one is the blue channel above.
		VOE_TEST_CHECK(drawn_pixels(pixels + FRONT_OFFSET, clear) <
			       SIDE * SIDE);

		voe_render_vk.unmap_memory(device->device, readback.memory);
	} else {
		VOE_TEST_CHECK(mapped != NULL);
	}

	// Idle before anything the GPU touched goes away. The device's own
	// teardown does this for what it owns; the buffer is this file's.
	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
