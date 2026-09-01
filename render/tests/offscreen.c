// BACK FACES ARE CULLED, AND THE Y FLIP, THE WINDING AND THE FRONT-FACE CONSTANT
// AGREE ABOUT WHICH WAY ROUND THAT IS. Three separate lines in three files say
// one thing between them — the negative viewport height in
// voe_render_frame_viewport, the counter-clockwise triangle in triangle.slang,
// and VK_FRONT_FACE_CLOCKWISE beside VK_CULL_MODE_BACK_BIT in device.c — and
// none of them means anything alone. CLAUDE.md says flipping twice looks exactly
// like flipping none until something is culled; this is the test that notices.
//
// HOW A BACK FACE GETS IN FRONT OF THE RASTERISER WITHOUT A SECOND SHADER. The
// viewport is dynamic state, so the same triangle drawn through the mirror of
// the engine's viewport is wound the other way round in framebuffer space and is
// a back face by definition. That leaves the pipeline — the thing actually under
// test — untouched, and it means the front-facing half of this test runs the
// engine's own viewport function rather than a copy of it. Change either
// constant in device.c, or the sign in voe_render_frame_viewport, and the
// front-facing draw disappears and this fails.
//
// IT IS NOT ONLY A CENTRE PIXEL, BECAUSE THAT WOULD MISS THE DOUBLE FLIP. A
// triangle standing on its head still covers the middle of the image. So the
// samples below are asymmetric in Y: above the apex, below the base, and one
// inside the bottom-left corner where the triangle is wide — a point that is
// outside it the moment the picture is upside down. The colour there is checked
// too, because the bottom-left vertex is the green one, which pins left against
// right as well.
//
// IT RUNS HEADLESS AND THAT IS WHY IT CAN BE A TEST AT ALL. Drawing into an
// offscreen image needs no window, no compositor and no surface, so this runs
// under ctest on a machine with nothing but a driver — see
// voe_render_device_new_headless in device_internal.h. Until the frame went into
// an image of its own there was nothing to read back and this had to be a person
// looking at a window.
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

// Everything this test needs that the engine does not. render's table is the set
// of functions render's own code calls, and reading an image back into memory is
// not one of them — so these are resolved here, by hand, out of the same device.
// Putting them in the table would be adding surface for a test, which rule 10 is
// there to stop.
static struct {
	PFN_vkCreateBuffer create_buffer;
	PFN_vkDestroyBuffer destroy_buffer;
	PFN_vkGetBufferMemoryRequirements get_buffer_memory_requirements;
	PFN_vkBindBufferMemory bind_buffer_memory;
	PFN_vkMapMemory map_memory;
	PFN_vkUnmapMemory unmap_memory;
	PFN_vkCmdCopyImageToBuffer cmd_copy_image_to_buffer;
} readback;

#define RESOLVE(field, vkname)                                                 \
	(readback.field = (PFN_##vkname)voe_render_vk.get_device_proc_addr(    \
		 device->device, #vkname))

static bool resolve_readback(voe_render_device *device)
{
	RESOLVE(create_buffer, vkCreateBuffer);
	RESOLVE(destroy_buffer, vkDestroyBuffer);
	RESOLVE(get_buffer_memory_requirements, vkGetBufferMemoryRequirements);
	RESOLVE(bind_buffer_memory, vkBindBufferMemory);
	RESOLVE(map_memory, vkMapMemory);
	RESOLVE(unmap_memory, vkUnmapMemory);
	RESOLVE(cmd_copy_image_to_buffer, vkCmdCopyImageToBuffer);

	// One guard for all seven. Every one of them is core Vulkan 1.0, so a
	// driver missing any is broken rather than merely old — but a test that
	// called through a null pointer would die where it is supposed to
	// report, and the analyser is right to say so.
	return readback.create_buffer != NULL &&
	       readback.destroy_buffer != NULL &&
	       readback.get_buffer_memory_requirements != NULL &&
	       readback.bind_buffer_memory != NULL &&
	       readback.map_memory != NULL && readback.unmap_memory != NULL &&
	       readback.cmd_copy_image_to_buffer != NULL;
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
// then own and get wrong. Two targets already exist; using both costs nothing.
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

	// The engine's own drawing, which leaves the target in
	// TRANSFER_SRC_OPTIMAL — which is exactly the layout this copy wants.
	voe_render_frame_draw(device, frame, viewport);

	readback.cmd_copy_image_to_buffer(frame->commands, frame->target.image,
					  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					  buffer, 1, &region);
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
// to. Zero is the whole claim for the back-facing case: not "the middle is
// empty" but "nothing was drawn anywhere".
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

// The apex is at y = +0.6 and the base runs along y = -0.6 from x = -0.6 to
// x = +0.6, in a clip space where +Y is up. With the flip, +Y is the top of the
// image, so these five are: outside above, inside the middle, outside below, and
// inside near the wide bottom-left corner — plus one corner of the image, which
// is outside everything and so is the clear colour itself.
//
// The fourth is the one that catches a picture that is upside down: mirror the
// triangle in Y and that point lands next to the apex, where the triangle is
// 0.04 wide, and nothing is drawn there.
static void check_front(const unsigned char *front, const unsigned char *clear)
{
	const unsigned char *low_left = pixel_at(front, 14, 48);

	VOE_TEST_CHECK(!same_colour(pixel_at(front, SIDE / 2, SIDE / 2), clear));
	VOE_TEST_CHECK(same_colour(pixel_at(front, SIDE / 2, 5), clear));
	VOE_TEST_CHECK(same_colour(pixel_at(front, SIDE / 2, 58), clear));
	VOE_TEST_CHECK(!same_colour(low_left, clear));

	// The bottom-left vertex of the triangle is the green one, so a point
	// near it is mostly green. This is what pins left against right, which
	// nothing above does.
	VOE_TEST_CHECK(low_left[GREEN] > low_left[RED]);
	VOE_TEST_CHECK(low_left[GREEN] > low_left[BLUE]);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = IMAGE_BYTES * 2,
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	};
	uint32_t type;
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

	VOE_TEST_CHECK_INT(readback.create_buffer(device->device, &buffer_info,
						  NULL, &buffer),
			   VK_SUCCESS);
	readback.get_buffer_memory_requirements(device->device, buffer,
						&requirements);

	// Coherent as well as visible, so that reading it after the fence needs
	// no invalidate call and no third resolved function.
	type = voe_render_memory_type(device, requirements.memoryTypeBits,
				      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VOE_TEST_CHECK(type != UINT32_MAX);
	if (type == UINT32_MAX) {
		readback.destroy_buffer(device->device, buffer, NULL);
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = type;
	VOE_TEST_CHECK_INT(voe_render_vk.allocate_memory(device->device,
							 &allocate, NULL,
							 &memory),
			   VK_SUCCESS);
	VOE_TEST_CHECK_INT(readback.bind_buffer_memory(device->device, buffer,
						       memory, 0),
			   VK_SUCCESS);

	record_case(device, &device->frames[0],
		    voe_render_frame_viewport(device->resolution), buffer,
		    FRONT_OFFSET);
	record_case(device, &device->frames[1],
		    mirrored_viewport(device->resolution), buffer, BACK_OFFSET);

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

	VOE_TEST_CHECK_INT(readback.map_memory(device->device, memory, 0,
					       VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		pixels = mapped;

		// A corner of the front image, which is outside the triangle
		// whichever way up it is. Taking the clear colour from the
		// picture rather than from a constant means frame.c is free to
		// change it without touching this file.
		clear = pixel_at(pixels + FRONT_OFFSET, 1, 1);

		check_front(pixels + FRONT_OFFSET, clear);

		// The whole of the claim: a back-facing triangle leaves the
		// image exactly as the clear left it.
		VOE_TEST_CHECK_INT(drawn_pixels(pixels + BACK_OFFSET, clear), 0);

		// And the front one is not empty either, which is what stops the
		// line above from passing on a device that drew nothing at all.
		VOE_TEST_CHECK(drawn_pixels(pixels + FRONT_OFFSET, clear) > 0);

		readback.unmap_memory(device->device, memory);
	} else {
		VOE_TEST_CHECK(mapped != NULL);
	}

	// Idle before anything the GPU touched goes away. The device's own
	// teardown does this for what it owns; the buffer is this file's.
	voe_render_vk.device_wait_idle(device->device);
	readback.destroy_buffer(device->device, buffer, NULL);
	voe_render_vk.free_memory(device->device, memory, NULL);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
