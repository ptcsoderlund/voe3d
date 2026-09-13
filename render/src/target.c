// What a frame is drawn into: a colour image and a depth image, one pair per
// frame slot. This is what the engine renders to; the swapchain image is only
// where the colour half is copied at the very end, and nothing in this engine
// draws into one any more. See device_internal.h for why the split between this
// file and swapchain.c is where it is.
//
// WHY THERE IS AN IMAGE OF OUR OWN AT ALL. Drawing straight into the acquired
// swapchain image works, and it is what this folder did once. What it makes
// impossible is everything that wants to look at the frame after it is drawn and
// before it is on screen: tone mapping, any other post process, rendering at a
// resolution the window is not, and a viewport inside an editor. All four are
// the same missing thing — an image the engine owns — so it is one layer and not
// four.
//
// THE TARGET IS THE RESOLUTION AND THE SWAPCHAIN IS NOT. Both are built from the
// window's size today, so the two numbers agree and the copy at the end of a
// frame is one to one. They are still two numbers: what reconciles them is a
// blit, which scales, and that is what makes rendering at half the window's size
// a change to this file and nothing else.
//
// THE DEPTH IMAGE IS PER SLOT FOR THE SAME REASON THE COLOUR ONE IS, AND NOT
// BECAUSE IT IS READ LATER. The GPU may still be drawing the frame before last,
// and two frames in flight sharing one depth buffer would have one frame's
// depth test rejecting the other frame's fragments. It is the cheaper resource
// to share and it is still wrong to share. Nothing ever copies it anywhere:
// unlike the colour image it is written, tested against, and thrown away inside
// one frame.
//
// D32_SFLOAT AND NOTHING ELSE, BECAUSE DEPTH RUNS BACKWARDS HERE. The near plane
// is at 1.0 and the far plane at 0.0, so the distance is bunched near zero,
// which is exactly where a float has its precision and where a normalised
// integer format has least. VOE_RENDER_DEPTH_FORMAT says the same thing; the
// format is required of every Vulkan implementation as a depth attachment, so
// there is nothing to query and nothing to fall back to.
//
// ONE ALLOCATION PER IMAGE, AND THAT DOES NOT SCALE. A driver is allowed to
// refuse after a few thousand vkAllocateMemory calls and each one is expensive,
// so an engine that makes many images sub-allocates out of a few large blocks.
// This engine makes four. The allocator is a card of its own and writing it now
// would be writing it against nothing.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

uint32_t voe_render_memory_type(const voe_render_device *device, uint32_t mask,
				VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memory;

	VOE_BASE_DEBUG_ASSERT(device != NULL,
			      "asking for a memory type without a device");

	voe_render_vk.get_memory_properties(device->physical, &memory);

	// The first type that will do, which is what the specification tells a
	// caller to take: the types are ordered so that the earliest match is
	// the one with the fewest properties beyond what was asked for, and
	// asking for more than you need is how you end up on the slow heap.
	for (uint32_t i = 0; i < memory.memoryTypeCount; i++) {
		if ((mask & (1u << i)) == 0)
			continue;
		if ((memory.memoryTypes[i].propertyFlags & properties) ==
		    properties)
			return i;
	}
	return UINT32_MAX;
}

// One image, its memory and its view. Everything that differs between the
// colour target and the depth target is a parameter, so there is one copy of
// this and not two that drift apart. `what` appears only in the messages, and
// it is there because "vkCreateImage failed" without it does not say which of
// the two images a person should be looking at.
static bool build_image(voe_render_device *device,
			struct voe_render_allocated_image *out,
			VkExtent2D extent, VkFormat format,
			VkImageUsageFlags usage, VkImageAspectFlags aspect,
			const char *what)
{
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = { extent.width, extent.height, 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	};
	VkImageViewCreateInfo view = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = {
			.aspectMask = aspect,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	uint32_t type;
	VkResult result;

	result = voe_render_vk.create_image(device->device, &info, NULL,
					    &out->image);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateImage failed for a %ux%u %s target (VkResult %d)",
			       extent.width, extent.height, what, (int)result);
		out->image = VK_NULL_HANDLE;
		return false;
	}

	voe_render_vk.get_image_memory_requirements(device->device, out->image,
						    &requirements);

	// DEVICE_LOCAL and nothing else: both of these are written by the GPU
	// every frame and read by the CPU never. A card with no device-local
	// type the image will accept is refused with a message rather than
	// allocated somewhere slow behind the programmer's back.
	type = voe_render_memory_type(device, requirements.memoryTypeBits,
				      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (type == UINT32_MAX) {
		VOE_BASE_ERROR("render",
			       "this graphics card offers no device-local memory a %s target can live in",
			       what);
		return false;
	}

	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = type;
	result = voe_render_vk.allocate_memory(device->device, &allocate, NULL,
					       &out->memory);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateMemory failed for %llu bytes of %s target (VkResult %d)",
			       (unsigned long long)requirements.size, what,
			       (int)result);
		out->memory = VK_NULL_HANDLE;
		return false;
	}

	result = voe_render_vk.bind_image_memory(device->device, out->image,
						 out->memory, 0);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkBindImageMemory failed on a %s target (VkResult %d)",
			       what, (int)result);
		return false;
	}

	view.image = out->image;
	result = voe_render_vk.create_image_view(device->device, &view, NULL,
						 &out->view);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateImageView failed on a %s target (VkResult %d)",
			       what, (int)result);
		out->view = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

static bool build_one(voe_render_device *device,
		      struct voe_render_target *target, VkExtent2D extent)
{
	// COLOUR_ATTACHMENT because everything drawn goes into it, TRANSFER_SRC
	// because the last thing a frame does is blit out of it. Nothing samples
	// it yet — the day something does is the day a post-process pass exists
	// to do the sampling.
	if (!build_image(device, &target->colour, extent,
			 device->format.format,
			 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
				 VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			 VK_IMAGE_ASPECT_COLOR_BIT, "colour"))
		return false;

	// DEPTH_STENCIL_ATTACHMENT and nothing else. No TRANSFER_SRC, because
	// nothing copies a depth image anywhere in this engine, and no SAMPLED,
	// because nothing reads one — a shadow map or a depth-aware post process
	// is the card that adds one of those, and it adds it here.
	return build_image(device, &target->depth, extent,
			   VOE_RENDER_DEPTH_FORMAT,
			   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			   VK_IMAGE_ASPECT_DEPTH_BIT, "depth");
}

// One image's three handles, in the order that respects what lives inside what.
// Safe on a zeroed struct and on one whose build stopped part way, which is what
// lets voe_render_target_build undo a partial failure by calling teardown.
static void teardown_image(voe_render_device *device,
			   struct voe_render_allocated_image *image)
{
	if (image->view != VK_NULL_HANDLE)
		voe_render_vk.destroy_image_view(device->device, image->view,
						 NULL);
	if (image->image != VK_NULL_HANDLE)
		voe_render_vk.destroy_image(device->device, image->image, NULL);
	// Last, because the image was living in it.
	if (image->memory != VK_NULL_HANDLE)
		voe_render_vk.free_memory(device->device, image->memory, NULL);

	*image = (struct voe_render_allocated_image){ 0 };
}

void voe_render_target_teardown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "tearing down a NULL device");

	if (device->device == VK_NULL_HANDLE)
		return;

	// A target the GPU is still drawing into or still blitting out of is
	// not ours to take away, and a fence per slot does not say that for
	// every slot at once. Idle does.
	voe_render_vk.device_wait_idle(device->device);

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_target *target = &device->frames[i].target;

		teardown_image(device, &target->depth);
		teardown_image(device, &target->colour);
	}
	device->resolution = (VkExtent2D){ 0, 0 };
}

bool voe_render_target_build(voe_render_device *device, voe_platform_size size)
{
	VkExtent2D extent;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "building a target for nothing");

	voe_render_target_teardown(device);

	// A window with no area is a window nothing can be drawn for, and it is
	// not a failure — the same answer swapchain.c gives, for the same
	// reason. Nothing is built and the resolution stays zero, which is what
	// frame.c reads to know there is nothing to do.
	if (size.width <= 0 || size.height <= 0)
		return true;

	extent.width = (uint32_t)size.width;
	extent.height = (uint32_t)size.height;

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		if (!build_one(device, &device->frames[i].target, extent)) {
			voe_render_target_teardown(device);
			return false;
		}
	}

	device->resolution = extent;
	return true;
}
