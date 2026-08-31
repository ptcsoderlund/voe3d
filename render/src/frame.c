// One frame: wait for the last one, take an image, clear it, draw the triangle
// into it, present it. The clear is not a draw — it is the load operation
// dynamic rendering performs as it begins — so the only thing recorded between
// begin and end is the triangle.
//
// THE ONE Y FLIP IN THIS ENGINE IS THE VIEWPORT BELOW. Vulkan's clip space has
// +Y pointing down the screen and this engine has +Y up, and the whole of the
// reconciliation is a negative viewport height here. Never a negated row in a
// projection matrix, and never both: flipping twice looks exactly like flipping
// none until something is culled, and then it is a bug nobody can see. The
// front-face constant that goes with this flip is set on the pipeline in
// device.c.
//
// ONE FRAME IN FLIGHT, AND THAT IS THE WHOLE OF THE SYNCHRONISATION. A fence
// says the last submit has finished, which is what makes the command buffer and
// the acquired semaphore safe to use again; a semaphore per swapchain image says
// that image's drawing is done, which is what present waits on. Anything more —
// two frames in flight, a pool of command buffers — is frame pacing, and frame
// pacing is not this card.
//
// TWO BARRIERS, BOTH REQUIRED, BOTH synchronization2. An acquired image is in
// whatever layout the presentation engine left it, which is why the first one
// comes from UNDEFINED and the contents are not preserved; a clear overwrites
// every pixel, so nothing is lost. The second one hands it back in the layout
// present demands.
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

static void record(voe_render_device *device, uint32_t index)
{
	VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = device->images[index].image,
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
		.imageView = device->images[index].view,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .color = { .float32 = { CLEAR_RED, CLEAR_GREEN,
						       CLEAR_BLUE, 1.0f } } },
	};
	VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .extent = device->extent },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colour,
	};
	// y at the bottom and a negative height: the flip, and the only one. The
	// depth range is the plain 0..1 identity — this engine's reversed depth
	// lives in the projection matrix that puts the near plane at 1.0, not
	// here, and there is no depth buffer in this frame to apply it to.
	VkViewport viewport = {
		.y = (float)device->extent.height,
		.width = (float)device->extent.width,
		.height = -(float)device->extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	// The scissor is the whole image and takes no part in the flip. It is in
	// framebuffer coordinates, which have no sign to get wrong.
	VkRect2D scissor = { .extent = device->extent };

	voe_render_vk.begin_command_buffer(device->commands, &begin);

	voe_render_vk.cmd_pipeline_barrier2(device->commands, &dependency);

	// The clear is the load operation, so it has already happened by the time
	// the first command inside is recorded. Three vertices and no buffer:
	// the positions and the colours are constants in the shader, looked up by
	// the index Vulkan hands each invocation.
	voe_render_vk.cmd_begin_rendering(device->commands, &rendering);
	voe_render_vk.cmd_set_viewport(device->commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(device->commands, 0, 1, &scissor);
	voe_render_vk.cmd_bind_pipeline(device->commands,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					device->pipeline);
	voe_render_vk.cmd_draw(device->commands, 3, 1, 0, 0);
	voe_render_vk.cmd_end_rendering(device->commands);

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	barrier.dstAccessMask = 0;
	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	voe_render_vk.cmd_pipeline_barrier2(device->commands, &dependency);

	voe_render_vk.end_command_buffer(device->commands);
}

static bool submit(voe_render_device *device, uint32_t index)
{
	VkCommandBufferSubmitInfo commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = device->commands,
	};
	VkSemaphoreSubmitInfo wait = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = device->acquired,
		.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
	};
	VkSemaphoreSubmitInfo signal = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = device->images[index].drawn,
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
					     device->submitted);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "render: vkQueueSubmit2 failed (VkResult %d)\n",
			(int)result);
		return false;
	}
	return true;
}

bool voe_render_device_frame(voe_render_device *device, voe_platform_size size)
{
	uint32_t index = 0;
	VkResult result;
	VkPresentInfoKHR present = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.swapchainCount = 1,
		.pImageIndices = &index,
	};

	VOE_BASE_DEBUG_ASSERT(device != NULL, "drawing with a NULL device");

	// A window with no area has no images and nothing to present. Skipped,
	// not failed — it comes back the moment the window does.
	if (size.width <= 0 || size.height <= 0)
		return true;

	if (device->rebuild || device->swapchain == VK_NULL_HANDLE ||
	    size.width != device->built.width ||
	    size.height != device->built.height) {
		if (!voe_render_swapchain_build(device, size))
			return false;
	}
	if (device->swapchain == VK_NULL_HANDLE)
		return true;

	// The fence is waited on before the acquire and reset after it, so that
	// an acquire that fails leaves the fence signalled. Resetting first
	// would leave a fence nothing will ever signal, and the next frame would
	// wait on it forever.
	voe_render_vk.wait_for_fences(device->device, 1, &device->submitted,
				      VK_TRUE, UINT64_MAX);

	result = voe_render_vk.acquire_next_image(device->device,
						  device->swapchain, UINT64_MAX,
						  device->acquired,
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

	voe_render_vk.reset_fences(device->device, 1, &device->submitted);
	voe_render_vk.reset_command_buffer(device->commands, 0);
	record(device, index);
	if (!submit(device, index))
		return false;

	present.pWaitSemaphores = &device->images[index].drawn;
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
