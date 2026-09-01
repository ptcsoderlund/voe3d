// The innards of voe_render_device, shared by the four files that make one:
// device.c starts it, target.c makes the images the scene is drawn into,
// swapchain.c builds the images the window is made of, and frame.c draws.
// Nothing outside render/src sees this.
//
// The split is by lifetime, not by subject. What is made once at startup and
// lives until shutdown is device.c's; what is thrown away and rebuilt every time
// the window changes size is target.c's and swapchain.c's; what happens between
// two presents is frame.c's. A reader chasing a leak or a resize bug knows which
// file to open from that sentence alone.
//
// NOTHING DRAWS INTO A SWAPCHAIN IMAGE. The scene goes into an offscreen colour
// image of the engine's own, one per frame slot, and copying that into the
// acquired swapchain image is a separate last step. The swapchain is therefore
// no longer where the resolution is decided — see resolution below.
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

// The offscreen colour image one frame slot draws into, and the memory under it.
// This is what the engine renders to; the swapchain image is only where the
// result is copied at the end.
//
// IT IS COLOUR ONLY AND THAT IS DELIBERATE, NOT UNFINISHED. Nothing drawn today
// has depth, and a depth target nothing tests against is a target nothing reads.
// It arrives with the card that draws a cube, as another field here and on the
// same per-slot pattern.
//
// There is no extent in here because there is only ever one: every slot's target
// is built at the same size, and that size is device->resolution below. Two
// copies of one number is two chances for them to disagree.
struct voe_render_target {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};

// Everything with a one-frame lifetime, in one struct, one per frame slot. This
// is the shape: a per-frame resource — a uniform buffer, a descriptor set, a
// staging buffer — becomes a field here and is reached through the slot, and it
// needs no new array and no new index. The target below is the first thing to
// arrive that way.
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
//
// The target is per slot for exactly the reason the command buffer is: the GPU
// may still be reading the frame before last, so a single target shared by every
// slot would be written by one frame while another was still blitting it.
struct voe_render_frame {
	VkCommandBuffer commands;
	VkSemaphore acquired;
	VkFence submitted;
	struct voe_render_target target;
};

struct voe_render_device {
	VkInstance instance;
	VkDebugUtilsMessengerEXT messenger;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical;
	VkDevice device;
	VkQueue queue;
	uint32_t queue_family;

	// No window, no surface and no swapchain: the shape the offscreen test
	// runs on. Everything else is the same object, so this is read in the
	// four places where a surface would otherwise be asked a question —
	// which instance and device extensions to enable, which queue families
	// can present, and where the format comes from — and nowhere else.
	bool headless;

	// Chosen once, and kept. It is the surface's format where there is a
	// surface, because the last thing a frame does is copy into a swapchain
	// image; the targets take the same one.
	//
	// THE TARGET SHARING THE SWAPCHAIN'S FORMAT IS A STARTING POINT. What
	// makes tone mapping possible at all is a target of higher precision
	// than the screen, and that is the card that introduces it. Until then
	// one format is one fewer thing to convert and the blit is a straight
	// copy.
	VkSurfaceFormatKHR format;

	// The triangle's pipeline, and the layout it needs in order to exist.
	// Startup's, not the swapchain's: the viewport and the scissor are
	// dynamic state, so a resize changes neither of these and there is
	// nothing here to rebuild. The layout is empty — the shader takes no
	// descriptors and no push constants — and Vulkan still wants one.
	VkPipelineLayout layout;
	VkPipeline pipeline;

	// The size every slot's target is, and the resolution the engine draws
	// at. It is the window's size today and it is not the swapchain's: what
	// reconciles the two is the blit at the end of a frame, which scales.
	// That is what makes rendering at a different resolution from the window
	// a change to this one number later on.
	VkExtent2D resolution;

	VkSwapchainKHR swapchain;
	VkExtent2D extent;

	// The window size the targets and the swapchain above were built for.
	// Compared against the size handed to every frame, and it is not always
	// the same number as the swapchain's extent: where a surface dictates
	// its own extent, the extent is the surface's answer and this is the
	// question that was asked. Keeping both is what stops a clamped size
	// from rebuilding on every frame forever.
	voe_platform_size built;
	uint32_t image_count;
	struct voe_render_image images[VOE_RENDER_MAX_IMAGES];

	// The frame slots, and the one pool every command buffer in them comes
	// out of. Startup's, apart from the target inside each one, which a
	// resize rebuilds.
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
// away. Neither does anything on a headless device.
[[nodiscard]] bool voe_render_swapchain_build(voe_render_device *device,
					      voe_platform_size size);
void voe_render_swapchain_teardown(voe_render_device *device);

// target.c. The offscreen images, one per frame slot, all built at size. Same
// contract as the pair above: safe on a device that never had any, and idle
// before anything is taken away.
[[nodiscard]] bool voe_render_target_build(voe_render_device *device,
					   voe_platform_size size);
void voe_render_target_teardown(voe_render_device *device);

// target.c, and used by render/tests/offscreen.c as well: the index of a memory
// type this card offers that is in mask and has every one of properties.
// UINT32_MAX when there is none — which is the driver's answer and so is
// reported by the caller, not asserted on here.
[[nodiscard]] uint32_t voe_render_memory_type(const voe_render_device *device,
					      uint32_t mask,
					      VkMemoryPropertyFlags properties);

// frame.c. The engine's viewport for a target of this size, and the one Y flip
// in the engine — read frame.c's header before touching it.
VkViewport voe_render_frame_viewport(VkExtent2D extent);

// frame.c. Clears a slot's target and draws the scene into it, leaving the
// target in TRANSFER_SRC_OPTIMAL and ready to be copied somewhere. The command
// buffer is the slot's and must already have been begun.
//
// THE VIEWPORT IS A PARAMETER BECAUSE THE TWO CALLERS HAND IN DIFFERENT ONES. A
// frame hands in voe_render_frame_viewport(); render/tests/offscreen.c hands in
// that one and then its mirror image, because the same triangle drawn through a
// mirrored viewport is wound the other way round in framebuffer space — which is
// how a back face gets in front of the rasteriser without a second shader and
// without touching the pipeline whose front-face constant is the thing under
// test.
void voe_render_frame_draw(voe_render_device *device,
			   const struct voe_render_frame *frame,
			   VkViewport viewport);

// device.c, used by swapchain.c: the format the surface was opened with, decided
// once because the surface does not change when the window resizes.
[[nodiscard]] bool voe_render_device_choose_format(voe_render_device *device,
						   voe_base_arena *arena);

// device.c. A device with no window: no surface, no swapchain, and neither of
// the extensions that need one. Everything else — the graphics card, the logical
// device, the pipeline, the frame slots and their targets — is the same code the
// windowed device runs, which is the whole point of it: a test on this is a test
// of what ships.
//
// It is here and not in render/device.h on purpose. Drawing into a target and
// reading it back needs no window and no compositor, which is what lets
// render/tests/offscreen.c run in ctest on a machine with no display; nothing
// outside render has asked to open a windowless device, and rule 10 says not to
// offer one until something does.
[[nodiscard]] voe_render_device *
voe_render_device_new_headless(voe_base_arena *arena, voe_platform_size size,
			       voe_base_error *error);
