// Textures: RGBA8 pixels from the CPU to an image the fragment stage samples,
// and the generational ids that name them (ADR-0018). See
// render/include/render/device.h for what a caller sees.
//
// THE UPLOAD IS THE BUFFER UPLOAD IN buffer.c WITH TWO LAYOUT TRANSITIONS ROUND
// IT, AND THAT IS THE WHOLE DIFFERENCE. A buffer's bytes can be copied straight
// into device-local memory; an image's cannot, because an image is in a layout
// and the layout says what may be done to it. So: staging buffer, move the image
// to TRANSFER_DST, copy, then move it to SHADER_READ_ONLY. Every one of those
// moves is a barrier and forgetting one is a validation error rather than a
// wrong picture, which is the good kind of mistake.
//
// MIPMAPS ARE GENERATED, AND THE CARD ASKED FOR THAT TO BE DECIDED EITHER WAY.
// They are generated, by blitting each level into the next one half its size —
// which is what a card that can filter linearly will do well and a card that
// cannot must not be asked to do at all, so the format is queried first and a
// card that says no gets a single level rather than a broken chain. The reason
// to have them is that this cube is flown around: a texture minified without
// mipmaps shimmers, and the shimmer is the sort of thing that gets blamed on the
// camera. One level is still correct, merely worse, which is what makes the
// fallback honest rather than a silent downgrade.
#include "device_internal.h"

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

// RGBA8 in the order assets hands it over, and UNORM rather than SRGB.
//
// UNORM IS A DECISION AND IT IS THE ONE THAT WILL BE REVISITED. An SRGB format
// makes the hardware un-gamma every texel on read, which is what a lighting
// model wants; there is no lighting model yet, so a texture drawn straight to
// the screen through an SRGB view would come out visibly pale against the same
// picture in any image viewer. Card 019 brings the light, and it is the card
// that makes this SRGB and gives the swapchain the matching treatment — both
// together or neither, because doing one is worse than doing nothing.
#define TEXTURE_FORMAT VK_FORMAT_R8G8B8A8_UNORM

// How many mip levels an image of this size has, counting the full-size one.
// Halving until a side reaches one, which is what the blit chain below does.
static uint32_t mip_levels_for(uint32_t width, uint32_t height)
{
	uint32_t side = width > height ? width : height;
	uint32_t levels = 1;

	while (side > 1) {
		side /= 2;
		levels++;
	}

	return levels;
}

// Whether this card will filter this format linearly, which is what a mipmap
// blit needs. Asked rather than assumed: R8G8B8A8_UNORM is required by Vulkan to
// be sampled, but linear *blit* filtering is a separate feature bit and a card
// is allowed to say no.
static bool can_generate_mipmaps(voe_render_device *device)
{
	VkFormatProperties properties;

	voe_render_vk.get_physical_device_format_properties(
		device->physical, TEXTURE_FORMAT, &properties);

	return (properties.optimalTilingFeatures &
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
}

// One image-layout transition on one mip level range.
//
// THE STAGE AND ACCESS MASKS ARE ALL_COMMANDS AND MEMORY_READ|WRITE, WHICH IS
// THE BLUNT ANSWER AND THE RIGHT ONE HERE. This runs once at startup with
// nothing else on the queue, so a precise mask would buy nothing measurable and
// cost the reader a table of which stage each transition really needs. The day
// textures are uploaded while frames are in flight is the day these become
// precise, and that day has a card.
static void transition(VkCommandBuffer commands, VkImage image,
		       VkImageLayout from, VkImageLayout to, uint32_t base_level,
		       uint32_t level_count)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT |
				 VK_ACCESS_2_MEMORY_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT |
				 VK_ACCESS_2_MEMORY_WRITE_BIT,
		.oldLayout = from,
		.newLayout = to,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = base_level,
			.levelCount = level_count,
			.layerCount = 1,
		},
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};

	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
}

// Blit level n-1 down into level n, for every level after the first. Each source
// level is moved to TRANSFER_SRC just before it is read, which is also what
// leaves every level but the last in the right layout for the final transition.
static void generate_mipmaps(VkCommandBuffer commands, VkImage image,
			     uint32_t width, uint32_t height, uint32_t levels)
{
	int32_t w = (int32_t)width;
	int32_t h = (int32_t)height;

	for (uint32_t level = 1; level < levels; level++) {
		int32_t next_w = w > 1 ? w / 2 : 1;
		int32_t next_h = h > 1 ? h / 2 : 1;
		VkImageBlit blit = {
			.srcSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = level - 1,
				.layerCount = 1,
			},
			.srcOffsets = { { 0, 0, 0 }, { w, h, 1 } },
			.dstSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = level,
				.layerCount = 1,
			},
			.dstOffsets = { { 0, 0, 0 }, { next_w, next_h, 1 } },
		};

		transition(commands, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, level - 1, 1);
		voe_render_vk.cmd_blit_image(
			commands, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
			VK_FILTER_LINEAR);

		w = next_w;
		h = next_h;
	}
}

// The image, its memory and its view. Close kin to build_image in target.c and
// deliberately not shared with it: that one is a render target, always one level,
// always device-local-and-nothing-else, and merging the two would make a
// function with a parameter for every way they differ.
static bool build_texture_image(voe_render_device *device,
				struct voe_render_texture_slot *slot,
				uint32_t width, uint32_t height,
				uint32_t levels)
{
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = TEXTURE_FORMAT,
		.extent = { width, height, 1 },
		.mipLevels = levels,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		// TRANSFER_SRC as well as DST, because generating mipmaps reads
		// this image back out of itself.
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			 VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			 VK_IMAGE_USAGE_SAMPLED_BIT,
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
		.format = TEXTURE_FORMAT,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = levels,
			.layerCount = 1,
		},
	};
	uint32_t type;
	VkResult result;

	result = voe_render_vk.create_image(device->device, &info, NULL,
					    &slot->image);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateImage failed for a %ux%u texture (VkResult %d)\n",
			width, height, (int)result);
		slot->image = VK_NULL_HANDLE;
		return false;
	}

	voe_render_vk.get_image_memory_requirements(device->device, slot->image,
						    &requirements);
	type = voe_render_memory_type(device, requirements.memoryTypeBits,
				      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (type == UINT32_MAX) {
		fprintf(stderr,
			"render: this graphics card offers no device-local memory a texture can live in\n");
		return false;
	}

	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = type;
	result = voe_render_vk.allocate_memory(device->device, &allocate, NULL,
					       &slot->memory);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkAllocateMemory failed for a %ux%u texture (VkResult %d)\n",
			width, height, (int)result);
		slot->memory = VK_NULL_HANDLE;
		return false;
	}

	result = voe_render_vk.bind_image_memory(device->device, slot->image,
						 slot->memory, 0);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkBindImageMemory failed on a texture (VkResult %d)\n",
			(int)result);
		return false;
	}

	view.image = slot->image;
	result = voe_render_vk.create_image_view(device->device, &view, NULL,
						 &slot->view);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateImageView failed on a texture (VkResult %d)\n",
			(int)result);
		slot->view = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

// Staging buffer in, image out: the transitions, the copy and the mipmap chain,
// recorded into one command buffer and waited on. The same shape as
// copy_and_wait in buffer.c and for the same reasons — see its header for why
// this idles rather than taking a fence.
static bool copy_into_image(voe_render_device *device,
			    struct voe_render_texture_slot *slot,
			    const struct voe_render_buffer *staging,
			    uint32_t width, uint32_t height, uint32_t levels)
{
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
	VkBufferImageCopy region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.imageExtent = { width, height, 1 },
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

	result = voe_render_vk.allocate_command_buffers(device->device,
							&allocate, &commands);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkAllocateCommandBuffers failed for a texture upload (VkResult %d)\n",
			(int)result);
		return false;
	}

	voe_render_vk.begin_command_buffer(commands, &begin);

	// Every level to TRANSFER_DST: level 0 receives the copy and the rest
	// receive their blits.
	transition(commands, slot->image, VK_IMAGE_LAYOUT_UNDEFINED,
		   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, levels);

	voe_render_vk.cmd_copy_buffer_to_image(
		commands, staging->buffer, slot->image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	generate_mipmaps(commands, slot->image, width, height, levels);

	// The chain leaves every level but the last in TRANSFER_SRC and the last
	// in TRANSFER_DST, so they are moved to the sampled layout in two
	// transitions rather than one. With a single level the first of these
	// covers nothing and the second does all the work, which is why the
	// count is computed rather than written as levels - 1.
	if (levels > 1)
		transition(commands, slot->image,
			   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0,
			   levels - 1);
	transition(commands, slot->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, levels - 1, 1);

	voe_render_vk.end_command_buffer(commands);

	submit_commands.commandBuffer = commands;
	result = voe_render_vk.queue_submit2(device->queue, 1, &submit,
					     VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkQueueSubmit2 failed for a texture upload (VkResult %d)\n",
			(int)result);
		voe_render_vk.free_command_buffers(device->device, device->pool,
						   1, &commands);
		return false;
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
	return true;
}

void voe_render_texture_write_descriptors(voe_render_device *device,
					  VkDescriptorSet set)
{
	VkDescriptorImageInfo images[VOE_RENDER_MAX_TEXTURES];
	VkWriteDescriptorSet write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstBinding = 1,
		.descriptorCount = VOE_RENDER_MAX_TEXTURES,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = images,
	};

	VOE_BASE_DEBUG_ASSERT(device != NULL, "writing descriptors with no device");

	// Slot 0 is the default texture and is always live, so an unclaimed slot
	// pointing at it is pointing at something valid. See
	// VOE_RENDER_MAX_TEXTURES in device_internal.h.
	for (uint32_t i = 0; i < VOE_RENDER_MAX_TEXTURES; i++) {
		const struct voe_render_texture_slot *slot =
			device->textures[i].live ? &device->textures[i]
						 : &device->textures[0];

		images[i] = (VkDescriptorImageInfo){
			.sampler = device->sampler,
			.imageView = slot->view,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
	}

	write.dstSet = set;
	voe_render_vk.update_descriptor_sets(device->device, 1, &write, 0, NULL);
}

bool voe_render_texture_create(voe_render_device *device, uint32_t width,
			       uint32_t height, const uint8_t *rgba,
			       voe_render_texture *out, voe_base_error *error)
{
	struct voe_render_buffer staging = { 0 };
	struct voe_render_texture_slot *slot = NULL;
	uint32_t index = 0;
	uint32_t levels;
	VkDeviceSize size;
	bool uploaded;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "creating a texture with no device");
	VOE_BASE_DEBUG_ASSERT(rgba != NULL, "creating a texture from nothing");
	VOE_BASE_DEBUG_ASSERT(out != NULL, "creating a texture into nothing");
	VOE_BASE_DEBUG_ASSERT(width > 0 && height > 0,
			      "creating a texture with no pixels in it");

	for (uint32_t i = 1; i < VOE_RENDER_MAX_TEXTURES; i++) {
		if (!device->textures[i].live) {
			index = i;
			slot = &device->textures[i];
			break;
		}
	}
	if (slot == NULL) {
		fprintf(stderr,
			"render: all %d texture slots are taken\n",
			VOE_RENDER_MAX_TEXTURES);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	levels = can_generate_mipmaps(device) ? mip_levels_for(width, height)
					      : 1;
	size = (VkDeviceSize)width * height * 4;

	if (!build_texture_image(device, slot, width, height, levels))
		goto refused;

	if (!voe_render_buffer_build(device, &staging, size,
				     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		goto refused;

	{
		void *mapped = NULL;
		VkResult result = voe_render_vk.map_memory(
			device->device, staging.memory, 0, VK_WHOLE_SIZE, 0,
			&mapped);

		if (result != VK_SUCCESS || mapped == NULL) {
			fprintf(stderr,
				"render: vkMapMemory failed on a texture staging buffer (VkResult %d)\n",
				(int)result);
			voe_render_buffer_teardown(device, &staging);
			goto refused;
		}
		memcpy(mapped, rgba, (size_t)size);
		voe_render_vk.unmap_memory(device->device, staging.memory);
	}

	uploaded = copy_into_image(device, slot, &staging, width, height,
				   levels);
	voe_render_buffer_teardown(device, &staging);
	if (!uploaded)
		goto refused;

	// Claimed only once the upload has actually happened, so a failure part
	// way through leaves the slot free rather than live and empty.
	slot->generation++;
	slot->live = true;

	// The device has already idled inside copy_into_image, so rewriting the
	// sets no frame is reading is safe. See the header on
	// voe_render_texture_create.
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
		voe_render_texture_write_descriptors(device,
						     device->frames[i].descriptor);

	out->index = index;
	out->generation = slot->generation;
	return true;

refused:
	// Whatever was made before the failure goes back, and the slot stays
	// unclaimed. Destroying a VK_NULL_HANDLE is defined and does nothing,
	// which is what makes this one path rather than three.
	voe_render_vk.destroy_image_view(device->device, slot->view, NULL);
	voe_render_vk.destroy_image(device->device, slot->image, NULL);
	voe_render_vk.free_memory(device->device, slot->memory, NULL);
	slot->view = VK_NULL_HANDLE;
	slot->image = VK_NULL_HANDLE;
	slot->memory = VK_NULL_HANDLE;
	if (error != NULL)
		*error = VOE_BASE_ERROR_REFUSED;
	return false;
}

bool voe_render_device_set_texture(voe_render_device *device,
				   voe_render_texture texture)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "setting a texture on no device");

	if (texture.index >= VOE_RENDER_MAX_TEXTURES)
		return false;
	if (!device->textures[texture.index].live)
		return false;
	// THE GENERATION CHECK, WHICH IS THE WHOLE REASON THE ID HAS TWO HALVES.
	// Without it an id kept across a texture being destroyed would name
	// whatever claimed the slot next and draw it without complaint.
	if (device->textures[texture.index].generation != texture.generation)
		return false;

	device->current_texture = texture;
	return true;
}

bool voe_render_texture_startup(voe_render_device *device)
{
	// One white pixel, so that an unclaimed slot samples to white rather
	// than to whatever was in memory — and so that a cube drawn before
	// anything has loaded shows its vertex colours unchanged, the fragment
	// stage multiplying by one.
	static const uint8_t WHITE[4] = { 0xff, 0xff, 0xff, 0xff };
	VkSamplerCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		// REPEAT because a texture on a cube face runs 0..1 exactly and
		// what happens outside that is a question nothing asks yet.
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		// Anisotropy is off: it is a device feature that has to be asked
		// for at device creation and nothing has asked. The card that
		// wants it enables the feature and sets these two together.
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1.0f,
		.minLod = 0.0f,
		.maxLod = VK_LOD_CLAMP_NONE,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
	};
	VkResult result;
	struct voe_render_texture_slot *slot = &device->textures[0];

	VOE_BASE_DEBUG_ASSERT(device != NULL, "starting textures on no device");

	result = voe_render_vk.create_sampler(device->device, &info, NULL,
					      &device->sampler);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "render: vkCreateSampler failed (VkResult %d)\n",
			(int)result);
		device->sampler = VK_NULL_HANDLE;
		return false;
	}

	// Slot 0 is the default and is built here rather than through
	// voe_render_texture_create, which starts its search at 1 precisely so
	// that this one cannot be handed out.
	{
		struct voe_render_buffer staging = { 0 };
		void *mapped = NULL;
		bool ok;

		if (!build_texture_image(device, slot, 1, 1, 1))
			return false;
		if (!voe_render_buffer_build(
			    device, &staging, sizeof(WHITE),
			    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;

		result = voe_render_vk.map_memory(device->device,
						  staging.memory, 0,
						  VK_WHOLE_SIZE, 0, &mapped);
		if (result != VK_SUCCESS || mapped == NULL) {
			fprintf(stderr,
				"render: vkMapMemory failed on the default texture (VkResult %d)\n",
				(int)result);
			voe_render_buffer_teardown(device, &staging);
			return false;
		}
		memcpy(mapped, WHITE, sizeof(WHITE));
		voe_render_vk.unmap_memory(device->device, staging.memory);

		ok = copy_into_image(device, slot, &staging, 1, 1, 1);
		voe_render_buffer_teardown(device, &staging);
		if (!ok)
			return false;
	}

	slot->generation = 1;
	slot->live = true;
	device->current_texture = (voe_render_texture){ .index = 0,
						       .generation = 1 };
	return true;
}

void voe_render_texture_shutdown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "shutting down textures on no device");

	for (uint32_t i = 0; i < VOE_RENDER_MAX_TEXTURES; i++) {
		struct voe_render_texture_slot *slot = &device->textures[i];

		voe_render_vk.destroy_image_view(device->device, slot->view,
						 NULL);
		voe_render_vk.destroy_image(device->device, slot->image, NULL);
		voe_render_vk.free_memory(device->device, slot->memory, NULL);
		slot->view = VK_NULL_HANDLE;
		slot->image = VK_NULL_HANDLE;
		slot->memory = VK_NULL_HANDLE;
		slot->live = false;
	}

	voe_render_vk.destroy_sampler(device->device, device->sampler, NULL);
	device->sampler = VK_NULL_HANDLE;
}
