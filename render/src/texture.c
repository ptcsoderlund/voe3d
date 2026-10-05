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
// THERE ARE NO MIPMAPS AND NO LINEAR FILTERING, AND THAT IS THE ENGINE'S RULE
// RATHER THAN THIS FILE'S OPINION. Every texture is one level, sampled NEAREST,
// magnified and minified. Both of the things removed here were antialiasing —
// a mipmap chain exists to stop a minified texture shimmering, and a linear
// filter exists to stop a magnified one showing its texels — and this engine has
// decided it does not want antialiasing anywhere. A texture therefore shows its
// texels close up and shimmers at a distance, and both are the intended picture.
//
// WHAT THAT COSTS, WRITTEN DOWN SO NOBODY REDISCOVERS IT AS A BUG. A texture
// minified past about one texel per pixel aliases, and the aliasing moves as the
// camera moves. That is the thing mipmaps were for and it will look like a fault
// in the sampler to anybody who does not know. It is not: see
// voe_render_sampling.
//
// TWO SAMPLERS, ONE PER MODE, MADE ONCE AT STARTUP. A slot remembers which mode
// it was created with and the descriptor write reads that back, so the
// descriptor path is still one loop over the whole table and there is still no
// per-draw sampler anywhere. With filtering gone the two differ only in how they
// address outside 0..1 — see voe_render_sampling, whose names now say less than
// they did.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <string.h>

// RGBA8 in the order assets hands it over, in one of two formats.
//
// TWO FORMATS, BECAUSE A PICTURE OF A COLOUR AND A PICTURE OF NUMBERS ARE NOT
// THE SAME THING. A base colour map was authored by somebody looking at it, so
// its bytes carry the sRGB curve and the hardware has to take it off before
// anything multiplies by them: that is SRGB, and it is the format that makes
// linear lighting correct rather than approximately correct. A metalness,
// roughness, occlusion or normal map is numbers that were never a colour, and
// decoding one bends every value towards zero — a roughness byte of 128 would
// arrive as 0.21 instead of 0.5 and every surface in the scene would go shiny.
// The caller says which it has (voe_render_texture_kind) and this is the whole
// of what that choice does.
//
// IT WAS ONE FORMAT, UNORM, UNTIL THERE WAS A LIGHT. With nothing lit, a colour
// texture read through an SRGB view came out visibly pale against the same
// picture in an image viewer, because nothing put the curve back on the way to
// the screen. Card 019's other half is the sRGB swapchain and target in
// device.c: the two had to change together and they did.
#define TEXTURE_COLOUR_FORMAT VK_FORMAT_R8G8B8A8_SRGB
#define TEXTURE_DATA_FORMAT VK_FORMAT_R8G8B8A8_UNORM

static VkFormat format_for(voe_render_texture_kind kind)
{
	return kind == VOE_RENDER_TEXTURE_COLOUR ? TEXTURE_COLOUR_FORMAT
						 : TEXTURE_DATA_FORMAT;
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

// The image, its memory and its view. Close kin to voe_render_target_image_build and
// deliberately not shared with it: that one is a render target, always one level,
// always device-local-and-nothing-else, and merging the two would make a
// function with a parameter for every way they differ.
static bool build_texture_image(voe_render_device *device,
				struct voe_render_texture_slot *slot,
				VkFormat format, uint32_t width,
				uint32_t height)
{
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = { width, height, 1 },
		// One level, always. There are no mipmaps in this engine — see
		// the header.
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		// TRANSFER_DST and no TRANSFER_SRC: the only thing that ever
		// read a texture back out of itself was the mipmap blit chain.
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
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
		.format = format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	uint32_t type;
	VkResult result;

	// Kept because it is the one thing about a slot that cannot be read back
	// off the image, and because it is what a reader of this file wants to
	// know first about a texture that came out wrong.
	slot->format = format;

	result = voe_render_vk.create_image(device->device, &info, NULL,
					    &slot->image);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateImage failed for a %ux%u texture (VkResult %d)",
			       width, height, (int)result);
		slot->image = VK_NULL_HANDLE;
		return false;
	}

	voe_render_vk.get_image_memory_requirements(device->device, slot->image,
						    &requirements);
	type = voe_render_memory_type(device, requirements.memoryTypeBits,
				      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (type == UINT32_MAX) {
		VOE_BASE_ERROR("render",
			       "this graphics card offers no device-local memory a texture can live in");
		return false;
	}

	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = type;
	result = voe_render_vk.allocate_memory(device->device, &allocate, NULL,
					       &slot->memory);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateMemory failed for a %ux%u texture (VkResult %d)",
			       width, height, (int)result);
		slot->memory = VK_NULL_HANDLE;
		return false;
	}

	result = voe_render_vk.bind_image_memory(device->device, slot->image,
						 slot->memory, 0);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkBindImageMemory failed on a texture (VkResult %d)",
			       (int)result);
		return false;
	}

	view.image = slot->image;
	result = voe_render_vk.create_image_view(device->device, &view, NULL,
						 &slot->view);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateImageView failed on a texture (VkResult %d)",
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
			    uint32_t width, uint32_t height)
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
		VOE_BASE_ERROR("render",
			       "vkAllocateCommandBuffers failed for a texture upload (VkResult %d)",
			       (int)result);
		return false;
	}

	voe_render_vk.begin_command_buffer(commands, &begin);

	// One level in, one level out. This was a chain of transitions around a
	// blit chain until the engine stopped having mipmaps.
	transition(commands, slot->image, VK_IMAGE_LAYOUT_UNDEFINED,
		   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1);

	voe_render_vk.cmd_copy_buffer_to_image(
		commands, staging->buffer, slot->image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	transition(commands, slot->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);

	voe_render_vk.end_command_buffer(commands);

	submit_commands.commandBuffer = commands;
	result = voe_render_vk.queue_submit2(device->queue, 1, &submit,
					     VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkQueueSubmit2 failed for a texture upload (VkResult %d)",
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
					  uint32_t slot)
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
	VOE_BASE_DEBUG_ASSERT(slot < VOE_RENDER_FRAMES_IN_FLIGHT,
			      "writing the descriptors of something that is not a frame slot");

	// Slot 0 is the default texture and is always live, so an unclaimed slot
	// pointing at it is pointing at something valid. See
	// VOE_RENDER_MAX_TEXTURES in device_internal.h.
	for (uint32_t i = 0; i < VOE_RENDER_MAX_TEXTURES; i++) {
		const struct voe_render_texture_slot *texture =
			device->textures[i].live ? &device->textures[i]
						 : &device->textures[0];
		VkImageView view = texture->view;
		VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// A target's picture: this frame slot's own colour image, which
		// is the whole of how a frame in slot n comes to read slot n's
		// picture through an id that is the same in every slot — and in
		// GENERAL, the one layout that image is ever in. See target_own.c.
		if (texture->is_target && !texture->depth) {
			view = device->targets[texture->target]
				       .images[slot]
				       .colour.view;
			layout = VK_IMAGE_LAYOUT_GENERAL;
		}

		// A depth copy, which rests in SHADER_READ_ONLY_OPTIMAL: the
		// window's or a target's, this frame slot's own — and white while
		// the window has no images, its area nought.
		if (texture->is_target && texture->depth) {
			const struct voe_render_target *images =
				texture->target == VOE_RENDER_WINDOW_DEPTH ?
					&device->frames[slot].target :
					&device->targets[texture->target]
						 .images[slot];

			view = images->depth_copy.view;
			if (view == VK_NULL_HANDLE)
				view = device->textures[0].view;
		}

		// The slot's own mode, which for an unclaimed slot is slot 0's
		// — the substitution above picked the slot and the sampler
		// comes from whichever slot that turned out to be.
		images[i] = (VkDescriptorImageInfo){
			.sampler = device->samplers[texture->sampling],
			.imageView = view,
			.imageLayout = layout,
		};
	}

	write.dstSet = device->frames[slot].descriptor;
	voe_render_vk.update_descriptor_sets(device->device, 1, &write, 0, NULL);
}

bool voe_render_texture_create(voe_render_device *device,
			       voe_render_texture_kind kind,
			       voe_render_sampling sampling, uint32_t width,
			       uint32_t height, const uint8_t *rgba,
			       voe_render_texture *out, voe_base_error *error)
{
	struct voe_render_buffer staging = { 0 };
	struct voe_render_texture_slot *slot = NULL;
	VkFormat format = format_for(kind);
	uint32_t index = 0;
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
		VOE_BASE_ERROR("render",
			       "all %d texture slots are taken",
			       VOE_RENDER_MAX_TEXTURES);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	size = (VkDeviceSize)width * height * 4;

	if (!build_texture_image(device, slot, format, width, height))
		goto refused;

	if (!voe_render_buffer_build(device, &staging, size,
				     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				     "texture staging"))
		goto refused;

	{
		void *mapped = NULL;
		VkResult result = voe_render_vk.map_memory(
			device->device, staging.memory, 0, VK_WHOLE_SIZE, 0,
			&mapped);

		if (result != VK_SUCCESS || mapped == NULL) {
			VOE_BASE_ERROR("render",
				       "vkMapMemory failed on a texture staging buffer (VkResult %d)",
				       (int)result);
			voe_render_buffer_teardown(device, &staging);
			goto refused;
		}
		memcpy(mapped, rgba, (size_t)size);
		voe_render_vk.unmap_memory(device->device, staging.memory);
	}

	uploaded = copy_into_image(device, slot, &staging, width, height);
	voe_render_buffer_teardown(device, &staging);
	if (!uploaded)
		goto refused;

	// Claimed only once the upload has actually happened, so a failure part
	// way through leaves the slot free rather than live and empty. The mode
	// is kept because the descriptor write below runs again every time the
	// table changes and has to name the same sampler each time.
	slot->sampling = sampling;
	slot->generation++;
	slot->live = true;

	// The device has already idled inside copy_into_image, so rewriting the
	// sets no frame is reading is safe. See the header on
	// voe_render_texture_create.
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
		voe_render_texture_write_descriptors(device, i);

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

bool voe_render_texture_destroy(voe_render_device *device,
				voe_render_texture texture)
{
	struct voe_render_texture_slot *slot;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "destroying a texture on no device");

	// Slot 0 is the white default and is never handed out, so an id naming
	// it is either a zeroed struct or a mistake; either way there is nothing
	// of the caller's to destroy.
	if (texture.index == VOE_RENDER_NO_TEXTURE ||
	    texture.index >= VOE_RENDER_MAX_TEXTURES)
		return false;

	slot = &device->textures[texture.index];
	if (!slot->live || slot->generation != texture.generation)
		return false;
	VOE_BASE_ASSERT(!slot->is_target,
			"destroying a target's texture — the target owns the images it reads, and nothing destroys a target");

	// The image may be in a descriptor set a frame is still reading, and the
	// rewrite below may not happen while one is. This is the same wait
	// creating a texture does and the same reason.
	voe_render_vk.device_wait_idle(device->device);

	voe_render_vk.destroy_image_view(device->device, slot->view, NULL);
	voe_render_vk.destroy_image(device->device, slot->image, NULL);
	voe_render_vk.free_memory(device->device, slot->memory, NULL);
	slot->view = VK_NULL_HANDLE;
	slot->image = VK_NULL_HANDLE;
	slot->memory = VK_NULL_HANDLE;

	// The generation is bumped here as well as on a claim, so that the id
	// just destroyed is refused immediately rather than only once something
	// else takes the slot.
	slot->generation++;
	slot->live = false;

	// Back to the white default, so the array stays valid — every element
	// has to be a real descriptor whether or not the shader samples it.
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
		voe_render_texture_write_descriptors(device, i);
	return true;
}

bool voe_render_texture_startup(voe_render_device *device)
{
	// One white pixel, so that an unclaimed slot samples to white rather
	// than to whatever was in memory — and so that a cube drawn before
	// anything has loaded shows its vertex colours unchanged, the fragment
	// stage multiplying by one.
	//
	// IT STANDS IN FOR A MISSING TEXTURE OF EITHER KIND, AND WHITE IS THE
	// ONE COLOUR WHERE THAT WORKS. White is 1 under both formats — the sRGB
	// curve fixes both ends — so a material with no base colour map and a
	// material with no roughness map both multiply their factor by exactly
	// one. Any other default would have to be two textures. It is built in
	// the data format because that is the one that does no decoding, which
	// is the easier of the two to reason about when reading this.
	static const uint8_t WHITE[4] = { 0xff, 0xff, 0xff, 0xff };
	// One per voe_render_sampling, in that enum's order, so a slot's mode is
	// the subscript.
	//
	// NO PICTURE IS FILTERED AND ONE FIELD IS, AND BOTH HALVES OF THAT ARE
	// DELIBERATE. A linear filter over a picture is a blur and a mipmap
	// chain is a blur chosen in advance; both are antialiasing and card 026
	// removed them, so SMOOTH and SHARP are NEAREST over one level and the
	// only thing left to choose between them is what happens outside 0..1.
	//
	// A SIGNED DISTANCE FIELD IS NOT A PICTURE AND THE SAME SENTENCE IS
	// FALSE OF IT. Its texels are distances, not colours: interpolating
	// between two of them says where the outline crosses between the two
	// texel centres, which is information the field was written to carry
	// and point sampling throws away — a plateau per texel, and an edge
	// that can only land on a texel boundary. FIELD is that one case. It
	// moves the edge onto the outline; it does not soften it, because what
	// reads the field cuts it hard afterwards.
	//
	// NO CHAIN IN ANY OF THE THREE. maxLod is 0 everywhere and none is
	// generated or uploaded; nothing here is an opening for one.
	VkSamplerCreateInfo infos[VOE_RENDER_SAMPLING_COUNT] = {
		// REPEAT, because a texture on a cube face runs 0..1 exactly and
		// what happens outside it is a question nothing asks.
		[VOE_RENDER_SAMPLING_SMOOTH] = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_NEAREST,
			.minFilter = VK_FILTER_NEAREST,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.anisotropyEnable = VK_FALSE,
			.maxAnisotropy = 1.0f,
			.minLod = 0.0f,
			.maxLod = 0.0f,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		},
		// CLAMP_TO_EDGE, and it still earns its place with the filtering
		// gone: a sheet is not tiled, and REPEAT lets a coordinate a
		// hair outside the border wrap to the far side of the atlas and
		// fetch a different glyph entirely.
		[VOE_RENDER_SAMPLING_SHARP] = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_NEAREST,
			.minFilter = VK_FILTER_NEAREST,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.anisotropyEnable = VK_FALSE,
			.maxAnisotropy = 1.0f,
			.minLod = 0.0f,
			.maxLod = 0.0f,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		},
		// The only filtered sampler in the engine, and CLAMP_TO_EDGE
		// for the same reason SHARP has it: the sheet it serves is an
		// atlas. LINEAR here reconstructs where the outline falls
		// between texel centres — see the block above for why that is
		// not the blur card 026 removed.
		[VOE_RENDER_SAMPLING_FIELD] = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.anisotropyEnable = VK_FALSE,
			.maxAnisotropy = 1.0f,
			.minLod = 0.0f,
			.maxLod = 0.0f,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		},
	};
	VkResult result;
	struct voe_render_texture_slot *slot = &device->textures[0];

	VOE_BASE_DEBUG_ASSERT(device != NULL, "starting textures on no device");

	// Anisotropy is off in all three, and for two reasons: it is a device
	// feature nothing has asked for at device creation, and it is itself
	// antialiasing, so it is not coming back — not even alongside the one
	// linear filter above, which is a different thing entirely.
	for (uint32_t i = 0; i < VOE_RENDER_SAMPLING_COUNT; i++) {
		result = voe_render_vk.create_sampler(device->device, &infos[i],
						      NULL,
						      &device->samplers[i]);
		if (result != VK_SUCCESS) {
			VOE_BASE_ERROR("render",
				       "vkCreateSampler failed for sampling mode %u (VkResult %d)",
				       i, (int)result);
			device->samplers[i] = VK_NULL_HANDLE;
			return false;
		}
	}

	// Slot 0 is the default and is built here rather than through
	// voe_render_texture_create, which starts its search at 1 precisely so
	// that this one cannot be handed out.
	{
		struct voe_render_buffer staging = { 0 };
		void *mapped = NULL;
		bool ok;

		if (!build_texture_image(device, slot, TEXTURE_DATA_FORMAT, 1, 1))
			return false;
		if (!voe_render_buffer_build(
			    device, &staging, sizeof(WHITE),
			    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			    "white texture staging"))
			return false;

		result = voe_render_vk.map_memory(device->device,
						  staging.memory, 0,
						  VK_WHOLE_SIZE, 0, &mapped);
		if (result != VK_SUCCESS || mapped == NULL) {
			VOE_BASE_ERROR("render",
				       "vkMapMemory failed on the default texture (VkResult %d)",
				       (int)result);
			voe_render_buffer_teardown(device, &staging);
			return false;
		}
		memcpy(mapped, WHITE, sizeof(WHITE));
		voe_render_vk.unmap_memory(device->device, staging.memory);

		ok = copy_into_image(device, slot, &staging, 1, 1);
		voe_render_buffer_teardown(device, &staging);
		if (!ok)
			return false;
	}

	// SMOOTH, explicitly: every unclaimed slot points at this one, so this is
	// the sampler the whole descriptor array falls back to and it should be
	// the ordinary one rather than whatever zero happens to mean.
	slot->sampling = VOE_RENDER_SAMPLING_SMOOTH;
	slot->generation = 1;
	slot->live = true;
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

	for (uint32_t i = 0; i < VOE_RENDER_SAMPLING_COUNT; i++) {
		voe_render_vk.destroy_sampler(device->device,
					      device->samplers[i], NULL);
		device->samplers[i] = VK_NULL_HANDLE;
	}
}
