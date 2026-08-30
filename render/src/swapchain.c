// The swapchain: the images the window is actually made of, and the only part of
// a device that is thrown away and built again while the program runs. Every
// resize comes through here. See device_internal.h for why this is its own file.
//
// THE EXTENT IS THE SURFACE'S, NOT THE WINDOW'S, AND THAT IS NOT A CONTRADICTION.
// The window's size is what tells us to rebuild — it is the only thing that
// notices a resize, and on a compositor that hands over the titlebar's height
// when decorations go away it is the only signal there is. What the swapchain is
// then built at is what Vulkan will accept: where the surface reports a current
// extent, that extent is mandatory and anything else is a validation error, and
// where it reports the "you choose" value — which is what Wayland does, always —
// the window's size is used, clamped to what the surface will take.
//
// A ZERO EXTENT BUILDS NOTHING AND IS NOT A FAILURE. A minimised window has no
// images to draw into and no driver will make a swapchain for one. This leaves
// the swapchain absent and says so by leaving the handle null; frame.c skips a
// frame it has no swapchain for and tries again next time round.
#include "device_internal.h"

#include <base/assert.h>

#include <stdio.h>

// Every driver supports it and no driver may refuse it, so there is nothing to
// choose and no fallback to write. Presentation that does something other than
// wait for the display is a question for a card about frame pacing.
#define PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR

static uint32_t clamp_u32(uint32_t value, uint32_t low, uint32_t high)
{
	if (value < low)
		return low;
	if (value > high)
		return high;
	return value;
}

static VkExtent2D extent_for(const VkSurfaceCapabilitiesKHR *capabilities,
			     voe_platform_size size)
{
	VkExtent2D extent;

	// 0xFFFFFFFF is the surface saying the window system does not impose a
	// size and the client picks. Anything else is mandatory.
	if (capabilities->currentExtent.width != UINT32_MAX)
		return capabilities->currentExtent;

	extent.width = clamp_u32((uint32_t)size.width,
				 capabilities->minImageExtent.width,
				 capabilities->maxImageExtent.width);
	extent.height = clamp_u32((uint32_t)size.height,
				  capabilities->minImageExtent.height,
				  capabilities->maxImageExtent.height);
	return extent;
}

// One more than the minimum, so that a frame can be worked on while another is
// on screen, and never more than the surface allows. A maximum of zero means
// there is no maximum.
static uint32_t image_count_for(const VkSurfaceCapabilitiesKHR *capabilities)
{
	uint32_t wanted = capabilities->minImageCount + 1;

	if (capabilities->maxImageCount != 0 &&
	    wanted > capabilities->maxImageCount)
		wanted = capabilities->maxImageCount;
	return wanted;
}

// OPAQUE where it is offered, which is everywhere a window is not translucent.
// The first supported bit otherwise, because a surface must support at least
// one and refusing to draw over a detail nothing here has an opinion on would be
// the wrong trade.
static VkCompositeAlphaFlagBitsKHR
composite_alpha_for(const VkSurfaceCapabilitiesKHR *capabilities)
{
	VkCompositeAlphaFlagsKHR supported = capabilities->supportedCompositeAlpha;

	if (supported & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
		return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if (supported & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
		return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	if (supported & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
		return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	return VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
}

static bool collect_images(voe_render_device *device)
{
	VkImage images[VOE_RENDER_MAX_IMAGES];
	uint32_t count = 0;
	VkSemaphoreCreateInfo semaphore = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	if (voe_render_vk.get_swapchain_images(device->device, device->swapchain,
					       &count, NULL) != VK_SUCCESS) {
		fprintf(stderr, "render: vkGetSwapchainImagesKHR failed\n");
		return false;
	}

	// Refused rather than clamped: an image we made no view for is an image
	// an acquire can still hand back, and that is a crash rather than a
	// missing frame.
	if (count > VOE_RENDER_MAX_IMAGES) {
		fprintf(stderr,
			"render: the driver made a swapchain of %u images and this engine holds %d\n",
			count, VOE_RENDER_MAX_IMAGES);
		return false;
	}

	if (voe_render_vk.get_swapchain_images(device->device, device->swapchain,
					       &count, images) != VK_SUCCESS) {
		fprintf(stderr, "render: vkGetSwapchainImagesKHR failed\n");
		return false;
	}

	for (uint32_t i = 0; i < count; i++) {
		VkImageViewCreateInfo view = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = device->format.format,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		};

		device->images[i].image = images[i];
		if (voe_render_vk.create_image_view(device->device, &view, NULL,
						    &device->images[i].view) !=
		    VK_SUCCESS) {
			fprintf(stderr, "render: vkCreateImageView failed\n");
			return false;
		}
		if (voe_render_vk.create_semaphore(device->device, &semaphore,
						   NULL,
						   &device->images[i].drawn) !=
		    VK_SUCCESS) {
			fprintf(stderr, "render: vkCreateSemaphore failed\n");
			return false;
		}

		// Counted one at a time so that a failure halfway leaves exactly
		// the images that exist to be torn down, and not one more.
		device->image_count = i + 1;
	}

	return true;
}

void voe_render_swapchain_teardown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "tearing down a NULL device");

	if (device->device == VK_NULL_HANDLE)
		return;

	// Nothing below may be taken away while the GPU is still reading it, and
	// a present in flight is exactly that.
	voe_render_vk.device_wait_idle(device->device);

	for (uint32_t i = 0; i < device->image_count; i++) {
		if (device->images[i].view != VK_NULL_HANDLE)
			voe_render_vk.destroy_image_view(device->device,
							 device->images[i].view,
							 NULL);
		if (device->images[i].drawn != VK_NULL_HANDLE)
			voe_render_vk.destroy_semaphore(device->device,
							device->images[i].drawn,
							NULL);
		device->images[i] = (struct voe_render_image){ 0 };
	}
	device->image_count = 0;

	if (device->swapchain != VK_NULL_HANDLE)
		voe_render_vk.destroy_swapchain(device->device,
						device->swapchain, NULL);
	device->swapchain = VK_NULL_HANDLE;
	device->extent = (VkExtent2D){ 0, 0 };
}

bool voe_render_swapchain_build(voe_render_device *device,
				voe_platform_size size)
{
	VkSurfaceCapabilitiesKHR capabilities;
	VkExtent2D extent;
	VkSwapchainCreateInfoKHR info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.presentMode = PRESENT_MODE,
		.clipped = VK_TRUE,
	};
	VkResult result;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "building a swapchain for nothing");

	voe_render_swapchain_teardown(device);
	device->rebuild = false;
	device->built = size;

	if (voe_render_vk.get_surface_capabilities(device->physical,
						   device->surface,
						   &capabilities) != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed\n");
		return false;
	}

	extent = extent_for(&capabilities, size);
	if (extent.width == 0 || extent.height == 0)
		return true;

	info.surface = device->surface;
	info.minImageCount = image_count_for(&capabilities);
	info.imageFormat = device->format.format;
	info.imageColorSpace = device->format.colorSpace;
	info.imageExtent = extent;
	info.preTransform = capabilities.currentTransform;
	info.compositeAlpha = composite_alpha_for(&capabilities);

	result = voe_render_vk.create_swapchain(device->device, &info, NULL,
						&device->swapchain);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateSwapchainKHR failed at %ux%u (VkResult %d)\n",
			extent.width, extent.height, (int)result);
		device->swapchain = VK_NULL_HANDLE;
		return false;
	}

	device->extent = extent;
	if (!collect_images(device)) {
		voe_render_swapchain_teardown(device);
		return false;
	}

	return true;
}
