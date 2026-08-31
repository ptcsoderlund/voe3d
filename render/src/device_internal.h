// The innards of voe_render_device, shared by the three files that make one:
// device.c starts it, swapchain.c builds the images it draws into, and frame.c
// draws. Nothing outside render/src sees this.
//
// The split is by lifetime, not by subject. What is made once at startup and
// lives until shutdown is device.c's; what is thrown away and rebuilt every time
// the window changes size is swapchain.c's; what happens between two presents is
// frame.c's. A reader chasing a leak or a resize bug knows which file to open
// from that sentence alone.
#pragma once

#include "loader.h"

#include <base/arena.h>
#include <platform/window.h>
#include <render/device.h>

// The most images a swapchain here may have. FIFO presentation hands back three
// or four on every driver measured; the number exists so that the per-image
// arrays are members and a resize allocates and frees nothing. A driver that
// wants more than this is refused with a message rather than quietly clamped —
// clamping would leave images we never made a view for and an acquire that
// returns an index we cannot draw to.
#define VOE_RENDER_MAX_IMAGES 8

// One swapchain image and the two things that belong to it for its whole life.
//
// drawn is per image and not per frame on purpose. vkQueuePresentKHR waits on it
// and there is no fence to say when that wait finished, so the only safe moment
// to reuse it is when the image it belongs to comes back out of an acquire —
// which is exactly when this one does.
struct voe_render_image {
	VkImage image;
	VkImageView view;
	VkSemaphore drawn;
};

struct voe_render_device {
	VkInstance instance;
	VkDebugUtilsMessengerEXT messenger;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical;
	VkDevice device;
	VkQueue queue;
	uint32_t queue_family;

	// Chosen once, from the surface, and kept: a swapchain rebuilt at a
	// different size is still the same surface and still takes the same
	// format.
	VkSurfaceFormatKHR format;

	// The triangle's pipeline, and the layout it needs in order to exist.
	// Startup's, not the swapchain's: the viewport and the scissor are
	// dynamic state, so a resize changes neither of these and there is
	// nothing here to rebuild. The layout is empty — the shader takes no
	// descriptors and no push constants — and Vulkan still wants one.
	VkPipelineLayout layout;
	VkPipeline pipeline;

	VkSwapchainKHR swapchain;
	VkExtent2D extent;

	// The window size the swapchain above was built for. Compared against
	// the size handed to every frame, and it is not always the same number
	// as the extent: where a surface dictates its own extent, the extent is
	// the surface's answer and this is the question that was asked. Keeping
	// both is what stops a clamped size from rebuilding the swapchain on
	// every frame forever.
	voe_platform_size built;
	uint32_t image_count;
	struct voe_render_image images[VOE_RENDER_MAX_IMAGES];

	// One frame in flight, so one of each. The fence is what makes reusing
	// the command buffer and the acquired semaphore safe: nothing is touched
	// until the submit that last used it has finished on the GPU.
	VkCommandPool pool;
	VkCommandBuffer commands;
	VkSemaphore acquired;
	VkFence submitted;

	// Set when a present said the swapchain no longer matches the surface.
	// The rebuild happens at the top of the next frame rather than here,
	// because the images the present is still reading are not ours to
	// destroy until the queue is idle.
	bool rebuild;
};

// swapchain.c. Both are safe to call on a device whose swapchain was never
// built, and _destroy waits for the device to go idle before it takes anything
// away.
[[nodiscard]] bool voe_render_swapchain_build(voe_render_device *device,
					      voe_platform_size size);
void voe_render_swapchain_teardown(voe_render_device *device);

// device.c, used by swapchain.c: the format the surface was opened with, decided
// once because the surface does not change when the window resizes.
[[nodiscard]] bool voe_render_device_choose_format(voe_render_device *device,
						   voe_base_arena *arena);
