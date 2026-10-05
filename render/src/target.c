// What a frame is drawn into: a colour image and a depth image, one pair per
// frame slot. This is what the engine renders to; the swapchain image is only
// where the colour half is copied at the very end, and nothing in this engine
// draws into one any more. See device_internal.h for why the split between this
// file and swapchain.c is where it is. The colour format is what encodes sRGB on
// the way out (device.h says why), so no file in this folder holds a gamma constant.
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
// to share and it is still wrong to share.
//
// EVERY DEPTH IMAGE HAS A SAMPLED COPY BESIDE IT (ADR-0305), filled only by
// voe_render_frame_copy_depth in pass.c and shown through one texture slot per
// target, the window's claimed by the first build here. The copy rests in
// SHADER_READ_ONLY_OPTIMAL from the moment it is made, so every descriptor naming
// it agrees with its image.
//
// The targets of a caller's own are target_own.c's and the read back into an
// arena is target_read.c's; both build on the image helpers here.
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
// this and not two that drift apart. `what` names the image and its view for a
// capture tool (debug_names.c) and appears in the messages, because
// "vkCreateImage failed" without it does not say which of the two images a
// person should be looking at.
//
// ONE ALLOCATION PER IMAGE, AND THAT DOES NOT SCALE. A driver is allowed to
// refuse after a few thousand vkAllocateMemory calls and each one is expensive,
// so an engine that makes many images sub-allocates out of a few large blocks.
// This engine makes four. The allocator is a card of its own and writing it now
// would be writing it against nothing.
bool voe_render_target_image_build(voe_render_device *device,
				   struct voe_render_allocated_image *out,
				   VkExtent2D extent, VkFormat format,
				   VkImageUsageFlags usage,
				   VkImageAspectFlags aspect, const char *what)
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
	voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE,
			      (uint64_t)out->image, what);

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
	voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE_VIEW,
			      (uint64_t)out->view, what);

	return true;
}

// D32_SFLOAT AND NOTHING ELSE, BECAUSE DEPTH RUNS BACKWARDS HERE. The near plane
// is at 1.0 and the far plane at 0.0, so the distance is bunched near zero,
// which is exactly where a float has its precision and where a normalised
// integer format has least. VOE_RENDER_DEPTH_FORMAT says the same thing; the
// format is required of every Vulkan implementation as a depth attachment, so
// there is nothing to query and nothing to fall back to.
static bool build_one(voe_render_device *device,
		      struct voe_render_target *target, VkExtent2D extent)
{
	// COLOUR_ATTACHMENT because everything drawn goes into it, TRANSFER_SRC
	// because the last thing a frame does is blit out of it. Nothing samples
	// it yet — the day something does is the day a post-process pass exists
	// to do the sampling.
	if (!voe_render_target_image_build(device, &target->colour, extent,
					   device->format.format,
					   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
						   VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
					   VK_IMAGE_ASPECT_COLOR_BIT, "colour"))
		return false;

	// DEPTH_STENCIL_ATTACHMENT, and TRANSFER_SRC because a pass may copy it
	// into the sampled copy beside it. Nothing samples it directly.
	if (!voe_render_target_image_build(device, &target->depth, extent,
					   VOE_RENDER_DEPTH_FORMAT,
					   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
						   VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
					   VK_IMAGE_ASPECT_DEPTH_BIT, "depth"))
		return false;

	return voe_render_target_depth_copy_build(device, &target->depth_copy,
						  extent, "depth copy");
}

bool voe_render_target_depth_copy_build(voe_render_device *device,
					struct voe_render_allocated_image *out,
					VkExtent2D extent, const char *what)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "building a depth copy on no device");
	VOE_BASE_DEBUG_ASSERT(out != NULL, "building a depth copy into nothing");

	// D32 is required of every implementation as a sampled image, so there
	// is nothing to ask the card.
	return voe_render_target_image_build(device, out, extent,
					     VOE_RENDER_DEPTH_FORMAT,
					     VK_IMAGE_USAGE_TRANSFER_DST_BIT |
						     VK_IMAGE_USAGE_SAMPLED_BIT,
					     VK_IMAGE_ASPECT_DEPTH_BIT, what);
}

VkImageMemoryBarrier2 voe_render_target_settle_copy(VkImage image)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	VOE_BASE_DEBUG_ASSERT(image != VK_NULL_HANDLE, "settling no depth copy");
	VOE_BASE_DEBUG_ASSERT(barrier.image == image, "settling the wrong image");
	return barrier;
}

// Each of `clears` cleared to nought in GENERAL, between the barriers either
// side of it; nothing when there are none.
static void record_clears(VkCommandBuffer commands,
			  const VkDependencyInfo *dependency,
			  const VkImage *clears, uint32_t clear_count)
{
	const VkClearColorValue zero = { 0 };
	const VkImageSubresourceRange range = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.levelCount = 1,
		.layerCount = 1,
	};
	VkMemoryBarrier2 written = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
				 VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
				 VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
	};
	VkDependencyInfo after = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &written,
	};

	VOE_BASE_DEBUG_ASSERT(dependency != NULL, "clearing with no barriers before");
	voe_render_vk.cmd_pipeline_barrier2(commands, dependency);
	if (clear_count == 0)
		return;
	VOE_BASE_DEBUG_ASSERT(clears != NULL, "clearing no images");
	for (uint32_t i = 0; i < clear_count; i++)
		voe_render_vk.cmd_clear_color_image(commands, clears[i],
						    VK_IMAGE_LAYOUT_GENERAL,
						    &zero, 1, &range);
	voe_render_vk.cmd_pipeline_barrier2(commands, &after);
}

// The one-shot shape copy_into_image in texture.c has, with its blunt masks for
// the reason that file gives: nothing else is on the queue, and this idles.
// `barriers`, then each of `clears` cleared; the two settles call it.
static bool submit_once(voe_render_device *device,
			const VkImageMemoryBarrier2 *barriers, uint32_t count,
			const VkImage *clears, uint32_t clear_count)
{
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = count,
		.pImageMemoryBarriers = barriers,
	};
	VkCommandBufferAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = device->pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer commands = VK_NULL_HANDLE;
	VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VkCommandBufferSubmitInfo submit_commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	};
	VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &submit_commands,
	};
	VkResult result;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "settling images on no device");
	VOE_BASE_DEBUG_ASSERT(barriers != NULL && count > 0,
			      "settling no images");

	result = voe_render_vk.allocate_command_buffers(device->device,
							&allocate, &commands);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateCommandBuffers failed settling a target (VkResult %d)",
			       (int)result);
		return false;
	}

	result = voe_render_vk.begin_command_buffer(commands, &begin);
	if (result == VK_SUCCESS) {
		record_clears(commands, &dependency, clears, clear_count);
		result = voe_render_vk.end_command_buffer(commands);
	}
	if (result == VK_SUCCESS) {
		submit_commands.commandBuffer = commands;
		result = voe_render_vk.queue_submit2(device->queue, 1, &submit,
						     VK_NULL_HANDLE);
	}
	if (result == VK_SUCCESS)
		result = voe_render_vk.device_wait_idle(device->device);
	if (result != VK_SUCCESS)
		VOE_BASE_ERROR("render",
			       "settling a target's images failed (VkResult %d)",
			       (int)result);

	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
	return result == VK_SUCCESS;
}

bool voe_render_target_settle(voe_render_device *device,
			      const VkImageMemoryBarrier2 *barriers,
			      uint32_t count)
{
	return submit_once(device, barriers, count, NULL, 0);
}

bool voe_render_target_settle_cleared(voe_render_device *device,
				      const VkImageMemoryBarrier2 *barriers,
				      uint32_t count, const VkImage *clears,
				      uint32_t clear_count)
{
	return submit_once(device, barriers, count, clears, clear_count);
}

uint32_t voe_render_target_free_texture(const voe_render_device *device,
					uint32_t skip)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "searching no device's textures");
	VOE_BASE_DEBUG_ASSERT(device->textures[0].live,
			      "searching textures before the white default exists");

	// From 1, the search voe_render_texture_create makes: slot 0 is white.
	for (uint32_t i = 1; i < VOE_RENDER_MAX_TEXTURES; i++)
		if (!device->textures[i].live && i != skip)
			return i;
	return 0;
}

// The window's copy slot, claimed once: it names each frame slot's own copy, so
// a rebuild only rewrites the descriptors. False with a line if none is free.
static bool claim_window_depth_texture(voe_render_device *device)
{
	struct voe_render_texture_slot *texture;
	uint32_t index;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "claiming a slot on no device");
	if (device->window_depth_texture != 0)
		return true;

	index = voe_render_target_free_texture(device, 0);
	if (index == 0) {
		VOE_BASE_ERROR("render",
			       "all %d texture slots are taken, and the window's depth copy needs one",
			       VOE_RENDER_MAX_TEXTURES);
		return false;
	}
	texture = &device->textures[index];
	VOE_BASE_DEBUG_ASSERT(!texture->live, "claiming a live texture slot");
	texture->sampling = VOE_RENDER_SAMPLING_SHARP;
	texture->is_target = true;
	texture->depth = true;
	texture->target = VOE_RENDER_WINDOW_DEPTH;
	texture->generation++;
	texture->live = true;
	device->window_depth_texture = index;
	return true;
}

// One image's three handles, in the order that respects what lives inside what.
// Safe on a zeroed struct and on one whose build stopped part way, which is what
// lets voe_render_target_build undo a partial failure by calling teardown.
void voe_render_target_image_teardown(voe_render_device *device,
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

		voe_render_target_image_teardown(device, &target->depth_copy);
		voe_render_target_image_teardown(device, &target->depth);
		voe_render_target_image_teardown(device, &target->colour);
	}
	device->resolution = (VkExtent2D){ 0, 0 };

	// Every one of them is UNDEFINED again, so no slot holds a window
	// picture a copy may name TRANSFER_SRC_OPTIMAL for. This is also the
	// "after a resize the picture is undefined" rule, kept in one place.
	device->frame_ended = false;
}

// The window's copy slot named in every frame slot's set: its own copy, or white
// while there is none. Idle already, because teardown waited.
static void write_window_descriptors(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "writing no device's descriptors");
	VOE_BASE_DEBUG_ASSERT(device->window_depth_texture != 0,
			      "writing descriptors before the window's copy slot");
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
		voe_render_texture_write_descriptors(device, i);
}

bool voe_render_target_build(voe_render_device *device, voe_platform_size size)
{
	VkImageMemoryBarrier2 settles[VOE_RENDER_FRAMES_IN_FLIGHT];
	VkExtent2D extent;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "building a target for nothing");

	voe_render_target_teardown(device);
	if (!claim_window_depth_texture(device))
		return false;

	// A window with no area is a window nothing can be drawn for, and it is
	// not a failure — the same answer swapchain.c gives, for the same
	// reason. Nothing is built and the resolution stays zero, which is what
	// frame.c reads to know there is nothing to do.
	if (size.width <= 0 || size.height <= 0) {
		write_window_descriptors(device);
		return true;
	}

	extent.width = (uint32_t)size.width;
	extent.height = (uint32_t)size.height;

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		if (!build_one(device, &device->frames[i].target, extent)) {
			voe_render_target_teardown(device);
			write_window_descriptors(device);
			return false;
		}
		settles[i] = voe_render_target_settle_copy(
			device->frames[i].target.depth_copy.image);
	}
	if (!voe_render_target_settle(device, settles,
				      VOE_RENDER_FRAMES_IN_FLIGHT)) {
		voe_render_target_teardown(device);
		write_window_descriptors(device);
		return false;
	}

	device->resolution = extent;
	write_window_descriptors(device);
	return true;
}
