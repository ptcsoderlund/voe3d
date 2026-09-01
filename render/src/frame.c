// One frame: wait for the last one on this slot, draw the scene into the slot's
// offscreen target, take a swapchain image, copy the target into it, present it.
// The clear is not a draw — it is the load operation dynamic rendering performs
// as it begins — so the only thing recorded inside the rendering is the triangle.
//
// NOTHING HERE DRAWS INTO A SWAPCHAIN IMAGE. The scene goes into an image the
// engine owns (target.c) and the swapchain image is written once, by a blit, as
// the last thing a frame does. That separation is what a post-process pass, a
// render resolution different from the window's, and an editor viewport all need
// in order to exist at all.
//
// THE COPY IS A BLIT AND THAT IS A STARTING POINT. vkCmdBlitImage is one call
// and it scales, which is everything this card needs. A full-screen quad becomes
// necessary the moment anything wants to run a shader between the target and the
// screen — tone mapping first — and that is the card that replaces this.
//
// THE ONE Y FLIP IN THIS ENGINE IS voe_render_frame_viewport BELOW. Vulkan's
// clip space has +Y pointing down the screen and this engine has +Y up, and the
// whole of the reconciliation is a negative viewport height there. Never a
// negated row in a projection matrix, and never both: flipping twice looks
// exactly like flipping none until something is culled, and then it is a bug
// nobody can see. The front-face constant that goes with this flip is set on the
// pipeline in device.c, and the pair of them is proven by render/tests/offscreen.c.
//
// TWO INDICES RUN THROUGH THIS FILE AND THEY ARE NOT INTERCHANGEABLE. A frame
// slot counts how far ahead the CPU is allowed to run and is bounded by
// VOE_RENDER_FRAMES_IN_FLIGHT; a swapchain image index is whatever the driver
// hands back from an acquire and is bounded by the image count it chose. They
// are often both 2 or 3 and that means nothing. Each array is reached through
// one accessor below, each accessor asserts its own bound, and past that point
// the code holds pointers and has no index left to confuse — which is the point
// of the shape, because the mistake is silent otherwise.
//
// THE FENCE WAIT IS FOR THE FRAME VOE_RENDER_FRAMES_IN_FLIGHT AGO, NOT THE LAST
// ONE, AND THAT GAP IS THE WHOLE OF THE OVERLAP. Waiting on this slot's fence
// leaves every frame submitted since it still running on the GPU; one slot would
// put the wait back on the previous frame and there would be no overlap left.
// What that fence makes safe is this slot's own command buffer, its own acquire
// semaphore and its own target, and nothing else.
//
// THE TWO SEMAPHORE KINDS HAVE DIFFERENT LIFETIMES. The acquire semaphore is per
// slot, guarded by the fence beside it. The rendering-finished semaphore is per
// swapchain image, because present is what waits on it and present hands back no
// fence to say when it stopped. See device_internal.h for the full reasoning;
// flattening the two is a race the validation layers do not reliably catch.
//
// THE ACQUIRE IS WAITED ON AT THE BLIT AND NOT BEFORE. The first thing a frame
// does to a swapchain image is now a transfer, not a colour write, and the scene
// does not touch that image at all — so drawing the target can start while the
// presentation engine is still finished with the image, and only the copy has to
// wait. The wait stage and the stage the swapchain image's barriers name are the
// same one on purpose; making them disagree is how a layout transition ends up
// ordered before the semaphore it depends on.
//
// FOUR BARRIERS, ALL synchronization2. Two put the target into the layout the
// next thing needs — drawn into, then read out of — and two do the same for the
// swapchain image, which arrives in whatever layout the presentation engine left
// it and leaves in the one present demands. Both images come from UNDEFINED,
// because in both cases every pixel is about to be overwritten and there is
// nothing to preserve.
//
// A SWAPCHAIN GOES STALE AND THAT IS ORDINARY. Out-of-date means the surface
// changed under us and the swapchain has to be built again; suboptimal means it
// still works but no longer matches. Both are answered by rebuilding, neither is
// an error to report, and both happen for real on a compositor that resizes the
// client area when the titlebar goes away.
#include "device_internal.h"

#include <base/assert.h>

#include <stdio.h>

// The colour behind the triangle. It is deliberately none of the three the
// triangle's corners are, so that a person looking at the window can tell the
// ground from the thing standing on it, and it is deliberately not the grey the
// deleted placeholder buffer used, so that nobody can wonder which of the two
// put it there.
#define CLEAR_RED 0.04f
#define CLEAR_GREEN 0.32f
#define CLEAR_BLUE 0.38f

// The only place a frame slot indexes anything, and the only place an image
// index does. Both asserts are the same mistake read from either end: a slot is
// not an image index and an image index is not a slot, and with arrays this
// short a swap lands in range as often as not. Everything downstream takes the
// pointer these return, so there is no second place to get it wrong.
static struct voe_render_frame *frame_at(voe_render_device *device, uint32_t slot)
{
	VOE_BASE_DEBUG_ASSERT(slot < VOE_RENDER_FRAMES_IN_FLIGHT,
			      "a per-slot array reached with something that is not a frame slot");
	return &device->frames[slot];
}

static struct voe_render_image *image_at(voe_render_device *device,
					 uint32_t index)
{
	VOE_BASE_DEBUG_ASSERT(index < device->image_count,
			      "a per-image array reached with something that is not a swapchain image index");
	return &device->images[index];
}

VkViewport voe_render_frame_viewport(VkExtent2D extent)
{
	// y at the bottom and a negative height: the flip, and the only one. The
	// depth range is the plain 0..1 identity — this engine's reversed depth
	// lives in the projection matrix that puts the near plane at 1.0, not
	// here, and there is no depth buffer in this frame to apply it to.
	VkViewport viewport = {
		.y = (float)extent.height,
		.width = (float)extent.width,
		.height = -(float)extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	return viewport;
}

void voe_render_frame_draw(voe_render_device *device,
			   const struct voe_render_frame *frame,
			   VkViewport viewport)
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
		.image = frame->target.image,
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
	VkRenderingAttachmentInfo colour = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = frame->target.view,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .color = { .float32 = { CLEAR_RED, CLEAR_GREEN,
						       CLEAR_BLUE, 1.0f } } },
	};
	VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .extent = device->resolution },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colour,
	};
	// The scissor is the whole target and takes no part in the flip. It is
	// in framebuffer coordinates, which have no sign to get wrong.
	VkRect2D scissor = { .extent = device->resolution };

	VOE_BASE_DEBUG_ASSERT(device != NULL, "drawing with a NULL device");
	VOE_BASE_DEBUG_ASSERT(frame->target.image != VK_NULL_HANDLE,
			      "drawing into a frame slot that has no target");

	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);

	// The clear is the load operation, so it has already happened by the time
	// the first command inside is recorded. Three vertices and no buffer:
	// the positions and the colours are constants in the shader, looked up by
	// the index Vulkan hands each invocation.
	voe_render_vk.cmd_begin_rendering(frame->commands, &rendering);
	voe_render_vk.cmd_set_viewport(frame->commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(frame->commands, 0, 1, &scissor);
	voe_render_vk.cmd_bind_pipeline(frame->commands,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					device->pipeline);
	voe_render_vk.cmd_draw(frame->commands, 3, 1, 0, 0);
	voe_render_vk.cmd_end_rendering(frame->commands);

	// Left ready to be copied out of, by whoever asked for the drawing. A
	// frame blits it into a swapchain image; the offscreen test copies it
	// into memory it can read. ALL_TRANSFER and not the blit alone, because
	// those are two different stages and this barrier has to cover both —
	// naming one of them leaves the other reading an image this dependency
	// does not reach, which synchronization validation reports and nothing
	// else does.
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
}

// The target onto the screen, and the whole of what the swapchain image is for.
// The two extents are the same number today and the blit still reads both, so
// that the day the target stops being the window's size this file needs no edit.
static void blit_to_screen(voe_render_device *device,
			   const struct voe_render_frame *frame,
			   const struct voe_render_image *image)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image->image,
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
	VkImageBlit region = {
		.srcSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.srcOffsets = { { 0, 0, 0 },
				{ (int32_t)device->resolution.width,
				  (int32_t)device->resolution.height, 1 } },
		.dstSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.dstOffsets = { { 0, 0, 0 },
				{ (int32_t)device->extent.width,
				  (int32_t)device->extent.height, 1 } },
	};

	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);

	// LINEAR, which does nothing at all while the two extents match and is
	// the right answer the moment they do not. NEAREST would be a decision
	// to look worse later for no gain now.
	voe_render_vk.cmd_blit_image(frame->commands, frame->target.image,
				     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				     image->image,
				     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
				     &region, VK_FILTER_LINEAR);

	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	barrier.dstAccessMask = 0;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
}

static void record(voe_render_device *device,
		   const struct voe_render_frame *frame,
		   const struct voe_render_image *image)
{
	VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	voe_render_vk.begin_command_buffer(frame->commands, &begin);
	voe_render_frame_draw(device, frame,
			      voe_render_frame_viewport(device->resolution));
	blit_to_screen(device, frame, image);
	voe_render_vk.end_command_buffer(frame->commands);
}

// Where the two lifetimes meet: this waits on the slot's acquire semaphore,
// signals the image's rendering-finished one, and signals the slot's fence.
// Three objects across two arrays, and both arrive as pointers so that neither
// array is indexed here at all.
static bool submit(voe_render_device *device,
		   const struct voe_render_frame *frame,
		   const struct voe_render_image *image)
{
	VkCommandBufferSubmitInfo commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = frame->commands,
	};
	// The blit, because that is the first thing in this submit that touches
	// the acquired image, and it is the stage the barriers around it name.
	// Everything before it draws into a target of our own and has no reason
	// to wait for the presentation engine at all.
	VkSemaphoreSubmitInfo wait = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = frame->acquired,
		.stageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
	};
	VkSemaphoreSubmitInfo signal = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = image->drawn,
		.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
	};
	VkSubmitInfo2 info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.waitSemaphoreInfoCount = 1,
		.pWaitSemaphoreInfos = &wait,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &commands,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = &signal,
	};
	VkResult result;

	result = voe_render_vk.queue_submit2(device->queue, 1, &info,
					     frame->submitted);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "render: vkQueueSubmit2 failed (VkResult %d)\n",
			(int)result);
		return false;
	}
	return true;
}

// The targets and the swapchain are built from the same window size and rebuilt
// together, because the target is the resolution and the swapchain is where it
// lands. Order matters only in that a target that cannot be made is a device
// that cannot draw, and there is no point building a swapchain for it.
static bool rebuild(voe_render_device *device, voe_platform_size size)
{
	if (!voe_render_target_build(device, size))
		return false;
	return voe_render_swapchain_build(device, size);
}

bool voe_render_device_frame(voe_render_device *device, voe_platform_size size)
{
	struct voe_render_frame *frame;
	struct voe_render_image *image;
	uint32_t index = 0;
	VkResult result;
	VkPresentInfoKHR present = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.swapchainCount = 1,
		.pImageIndices = &index,
	};

	VOE_BASE_DEBUG_ASSERT(device != NULL, "drawing with a NULL device");
	VOE_BASE_DEBUG_ASSERT(!device->headless,
			      "asking a device with no window for a frame");

	// A window with no area has no images and nothing to present. Skipped,
	// not failed — it comes back the moment the window does.
	if (size.width <= 0 || size.height <= 0)
		return true;

	if (device->rebuild || device->swapchain == VK_NULL_HANDLE ||
	    size.width != device->built.width ||
	    size.height != device->built.height) {
		if (!rebuild(device, size))
			return false;
	}
	if (device->swapchain == VK_NULL_HANDLE)
		return true;

	// The slot this frame is, and the last time anything is indexed until the
	// acquire below hands back an image.
	frame = frame_at(device, device->slot);

	// The fence is waited on before the acquire and reset after it, so that
	// an acquire that fails leaves the fence signalled. Resetting first
	// would leave a fence nothing will ever signal, and the next frame would
	// wait on it forever.
	//
	// This is the wait for the frame VOE_RENDER_FRAMES_IN_FLIGHT ago. Every
	// frame submitted since is still running, and that is where the overlap
	// this file exists for comes from.
	voe_render_vk.wait_for_fences(device->device, 1, &frame->submitted,
				      VK_TRUE, UINT64_MAX);

	// The fence just waited on and the semaphore about to be handed to the
	// acquire have to belong to the same slot, or the semaphore is being
	// reused while a submit may still be waiting on it. Asking the driver
	// says so where reading the code only claims it: a wait on some other
	// slot's fence leaves this one unsignalled and this fires, which is the
	// one shape of this bug that no validation layer reliably reports.
	VOE_BASE_DEBUG_ASSERT(voe_render_vk.get_fence_status(device->device,
							     frame->submitted) ==
			      VK_SUCCESS,
			      "acquiring on a slot whose last submit has not finished");

	result = voe_render_vk.acquire_next_image(device->device,
						  device->swapchain, UINT64_MAX,
						  frame->acquired,
						  VK_NULL_HANDLE, &index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		device->rebuild = true;
		return true;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		fprintf(stderr,
			"render: vkAcquireNextImageKHR failed (VkResult %d)\n",
			(int)result);
		return false;
	}
	// Suboptimal is still a usable image, so this frame is drawn and the
	// rebuild waits until it has been presented.
	if (result == VK_SUBOPTIMAL_KHR)
		device->rebuild = true;

	image = image_at(device, index);

	voe_render_vk.reset_fences(device->device, 1, &frame->submitted);
	voe_render_vk.reset_command_buffer(frame->commands, 0);
	record(device, frame, image);
	if (!submit(device, frame, image))
		return false;

	// The slot is spent the moment the submit lands, and not before: a frame
	// that turned back above — no swapchain, a stale one, an acquire that
	// found the surface gone — never put this slot in flight and has to come
	// back to it with its fence still signalled. Advancing here and not at
	// the end also covers the presents below that return early.
	device->slot = (device->slot + 1) % VOE_RENDER_FRAMES_IN_FLIGHT;

	present.pWaitSemaphores = &image->drawn;
	present.pSwapchains = &device->swapchain;
	result = voe_render_vk.queue_present(device->queue, &present);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		device->rebuild = true;
		return true;
	}
	if (result != VK_SUCCESS) {
		fprintf(stderr, "render: vkQueuePresentKHR failed (VkResult %d)\n",
			(int)result);
		return false;
	}

	return true;
}
