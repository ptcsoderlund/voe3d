// The offscreen colour image the scene is drawn into, one per frame slot. This
// is what a frame renders to; the swapchain image is only where the result is
// copied at the very end, and nothing in this engine draws into one any more.
// See device_internal.h for why the split between this file and swapchain.c is
// where it is.
//
// WHY THERE IS AN IMAGE OF OUR OWN AT ALL. Drawing straight into the acquired
// swapchain image works, and it is what this folder did until now. What it makes
// impossible is everything that wants to look at the frame after it is drawn and
// before it is on screen: tone mapping, any other post process, rendering at a
// resolution the window is not, and a viewport inside an editor. All four are
// the same missing thing — an image the engine owns — so it is one layer and not
// four, and it went in early because it is a rewrite rather than an addition.
//
// THE TARGET IS THE RESOLUTION AND THE SWAPCHAIN IS NOT. Both are built from the
// window's size today, so the two numbers agree and the copy at the end of a
// frame is one to one. They are still two numbers: what reconciles them is a
// blit, which scales, and that is what makes rendering at half the window's size
// a change to this file and nothing else.
//
// COLOUR ONLY, AND ON PURPOSE. Nothing drawn today has depth, and a depth target
// nothing tests against is memory with nothing reading it. It arrives with the
// card that draws a cube, as another handle on the slot and built here beside
// this one.
//
// ONE ALLOCATION PER IMAGE, AND THAT DOES NOT SCALE. A driver is allowed to
// refuse after a few thousand vkAllocateMemory calls and each one is expensive,
// so an engine that makes many images sub-allocates out of a few large blocks.
// This engine makes two. The allocator is a card of its own and writing it now
// would be writing it against nothing.
#include "device_internal.h"

#include <base/assert.h>

#include <stdio.h>

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

static bool build_one(voe_render_device *device,
		      struct voe_render_target *target, VkExtent2D extent)
{
	// TRANSFER_SRC because the last thing a frame does is blit out of this
	// image, and COLOR_ATTACHMENT because everything before that draws into
	// it. Nothing samples it yet — the day something does is the day a
	// post-process pass exists to do the sampling.
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = device->format.format,
		.extent = { extent.width, extent.height, 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
			 VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
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
		.format = device->format.format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	uint32_t type;
	VkResult result;

	result = voe_render_vk.create_image(device->device, &info, NULL,
					    &target->image);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateImage failed for a %ux%u target (VkResult %d)\n",
			extent.width, extent.height, (int)result);
		target->image = VK_NULL_HANDLE;
		return false;
	}

	voe_render_vk.get_image_memory_requirements(device->device,
						    target->image,
						    &requirements);

	// DEVICE_LOCAL and nothing else: this is drawn into by the GPU every
	// frame and read by the CPU never. A card with no device-local type the
	// image will accept is refused with a message rather than allocated
	// somewhere slow behind the programmer's back.
	type = voe_render_memory_type(device, requirements.memoryTypeBits,
				      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (type == UINT32_MAX) {
		fprintf(stderr,
			"render: this graphics card offers no device-local memory a colour target can live in\n");
		return false;
	}

	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = type;
	result = voe_render_vk.allocate_memory(device->device, &allocate, NULL,
					       &target->memory);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkAllocateMemory failed for %llu bytes of colour target (VkResult %d)\n",
			(unsigned long long)requirements.size, (int)result);
		target->memory = VK_NULL_HANDLE;
		return false;
	}

	result = voe_render_vk.bind_image_memory(device->device, target->image,
						 target->memory, 0);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkBindImageMemory failed (VkResult %d)\n",
			(int)result);
		return false;
	}

	view.image = target->image;
	result = voe_render_vk.create_image_view(device->device, &view, NULL,
						 &target->view);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateImageView failed on a colour target (VkResult %d)\n",
			(int)result);
		target->view = VK_NULL_HANDLE;
		return false;
	}

	return true;
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

		if (target->view != VK_NULL_HANDLE)
			voe_render_vk.destroy_image_view(device->device,
							 target->view, NULL);
		if (target->image != VK_NULL_HANDLE)
			voe_render_vk.destroy_image(device->device,
						    target->image, NULL);
		// Last, because the image was living in it.
		if (target->memory != VK_NULL_HANDLE)
			voe_render_vk.free_memory(device->device,
						  target->memory, NULL);
		*target = (struct voe_render_target){ 0 };
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
