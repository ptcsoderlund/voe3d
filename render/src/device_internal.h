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

// How many frames the CPU may have submitted and unfinished at once, and the
// length of every per-slot array in this engine.
//
// THE NUMBER IS NOT THE DECISION. Read this constant everywhere and never assume
// its value: a literal 2 anywhere that means "frames in flight" is a bug, and
// going to three has to be this line and nothing else.
//
// IT IS NOT THE SWAPCHAIN IMAGE COUNT AND NEVER STANDS IN FOR IT. That is a
// number the driver chooses from the surface, for reasons that have nothing to
// do with how far ahead the CPU may run. They are often both 2 or 3 and that is
// a coincidence; the first driver that reports a different minimum breaks
// anything that leant on it. See voe_render_frame below for the half of the
// synchronisation that follows this constant, and voe_render_image for the half
// that follows the images.
#define VOE_RENDER_FRAMES_IN_FLIGHT 2

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

// Everything with a one-frame lifetime, in one struct, one per frame slot. This
// is the shape: a per-frame resource — a uniform buffer, a descriptor set, a
// staging buffer — becomes a field here and is reached through the slot, and it
// needs no new array and no new index.
//
// THE TWO SEMAPHORE KINDS HAVE DIFFERENT LIFETIMES AND MUST NOT BE FLATTENED
// INTO ONE. acquired is here, per slot, because the fence beside it is what says
// the submit that last waited on it has finished — that is the only thing that
// makes it safe to hand to another acquire. drawn is not here: it is per image,
// on voe_render_image above, because present is what waits on it and present
// hands back no fence to say when it stopped. Moving either one to the other's
// array is a race the validation layers do not reliably catch — an intermittent
// hang on one driver and never on the machine it was written on.
//
// submitted is the fence for this slot's last submit, and waiting on it at the
// top of a frame is waiting for the frame VOE_RENDER_FRAMES_IN_FLIGHT ago, not
// the previous one. That gap is the whole of the overlap.
struct voe_render_frame {
	VkCommandBuffer commands;
	VkSemaphore acquired;
	VkFence submitted;
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

	// The frame slots, and the one pool every command buffer in them comes
	// out of. Startup's, not the swapchain's: none of it depends on the
	// images, so a resize rebuilds none of it.
	//
	// slot is the one the next frame will use, advanced modulo the constant
	// the moment a submit succeeds — because a submit is what puts a slot in
	// flight, and a frame that returns before submitting must come back to
	// the same slot with its fence still signalled.
	VkCommandPool pool;
	struct voe_render_frame frames[VOE_RENDER_FRAMES_IN_FLIGHT];
	uint32_t slot;

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
