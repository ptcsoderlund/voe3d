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
//
// AND IT HOLDS THE TARGETS OF A CALLER'S OWN (ADR-0148), which are the same pair
// made again: one colour and one depth image per frame slot, in the window's
// formats, at a size of their own. What differs is that the colour image is also
// sampled — its picture is shown through one ordinary texture slot — and that is
// the whole of what the rest of this file below the window's half is about.
//
// ONE TEXTURE SLOT, AND EACH FRAME SLOT'S DESCRIPTOR SET POINTS IT AT THAT FRAME
// SLOT'S IMAGE. There is already a set per frame slot, each holding the whole
// texture table, so no new binding, set or shader change was needed: the
// descriptor write reads the frame slot it is writing and names that slot's
// image for a target's texture. See voe_render_texture_write_descriptors.
//
// A TARGET'S COLOUR IMAGE IS IN GENERAL FOR ITS WHOLE LIFE, and the descriptor
// sets say so. It is put there as soon as it is made — before anything has drawn
// into it, which is why its picture is undefined and not its layout — and never
// moves again: frame.c attaches it in GENERAL and every set samples it in GENERAL.
//
// ONE LAYOUT AND NOT A BARRIER EACH WAY, BECAUSE A TARGET'S OWN DESCRIPTOR IS
// BOUND IN THE PASS THAT DRAWS INTO IT. The set a pass binds holds the whole
// texture table, the target's slot included, and both shaders index that table
// with a number read out of a record. The validation layers cannot tell which
// numbers a draw will read, so they check every element of the array against the
// image's layout at every draw: resting in SHADER_READ_ONLY_OPTIMAL and moved into
// COLOR_ATTACHMENT_OPTIMAL for the pass, the target's own element disagreed with
// its image on every draw into it, though nothing read it. That was measured on
// the first version of this file, not supposed. GENERAL is valid for both uses,
// so there is no disagreement to report — and what it may cost on a driver that
// does better with the specialised layouts is the price of that silence.
//
// WHAT GENERAL DOES NOT MAKE LEGAL IS A PICTURE READING ITSELF. A draw that
// actually samples the target it is drawing into is a feedback loop Vulkan leaves
// undefined, and the layers stay quiet about it here too; that is what the debug
// asserts in frame.c and element.c are for.
//
// A RESIZE IS APPLIED AT THE TOP OF A FRAME AND IT WAITS FOR THE CARD. Every
// slot's images are thrown away together, for the reason the window's are: a
// fence says one slot is finished and says nothing about the others.
//
// AND IT HOLDS THE WAY BACK (ADR-0156). voe_render_target_read, at the bottom,
// copies one target's finished picture into a caller's arena as RGBA8 with
// straight alpha: a host-visible staging buffer, a one-shot copy, an idle wait,
// and the channel swap and the divide by alpha on the way out. It reads the slot
// the frame that ended last drew into, which is the slot before device->slot,
// and both kinds of target go through it — the window's pair above and a
// caller's own — because a picture saved and a picture shown being the same
// picture is the point rather than a coincidence.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdlib.h>

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

	// Every one of them is UNDEFINED again, so no slot holds a window
	// picture a copy may name TRANSFER_SRC_OPTIMAL for. This is also the
	// "after a resize the picture is undefined" rule, kept in one place.
	device->frame_ended = false;
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

// --------------------------------------------------- targets of one's own

void voe_render_targets_startup(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "starting targets on no device");

	// Nought is a device that asked for none, and there is then no table at
	// all: voe_render_target_create looks at the capacity before the table.
	if (device->capacities.targets == 0)
		return;

	device->targets = calloc(device->capacities.targets,
				 sizeof(*device->targets));
	VOE_BASE_ASSERT(device->targets != NULL,
			"out of memory making room for a device's targets");
}

// Every frame slot's pair belonging to one target. Safe on a slot that was never
// built and on one whose build stopped part way.
static void teardown_target_images(voe_render_device *device,
				   struct voe_render_target_slot *target)
{
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		teardown_image(device, &target->images[i].depth);
		teardown_image(device, &target->images[i].colour);
	}
	target->extent = (VkExtent2D){ 0, 0 };
}

void voe_render_targets_shutdown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "shutting targets down on no device");

	if (device->targets == NULL)
		return;

	// The caller of this has already waited for the card; see close_down in
	// device.c.
	for (uint32_t i = 0; i < device->capacities.targets; i++)
		teardown_target_images(device, &device->targets[i]);

	free(device->targets);
	device->targets = NULL;
}

struct voe_render_target_slot *voe_render_target_at(voe_render_device *device,
						    voe_render_target target)
{
	struct voe_render_target_slot *slot;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "asking no device for a target");

	// Index 0 is the window's, which is not in the table; the table's first
	// row is id index 1, so a zeroed id can never name a target.
	if (target.index == 0 || target.index > device->capacities.targets)
		return NULL;

	slot = &device->targets[target.index - 1];
	if (!slot->live || slot->generation != target.generation)
		return NULL;
	return slot;
}

// Moves every frame slot's colour image of one target out of UNDEFINED and into
// GENERAL, the layout the descriptor sets say it is in, and waits for that to
// happen. The
// same one-shot shape as copy_into_image in texture.c, and the same blunt masks
// for the reason that file's transition gives: nothing else is on the queue.
static bool settle(voe_render_device *device,
		   const struct voe_render_target_slot *target)
{
	VkImageMemoryBarrier2 barriers[VOE_RENDER_FRAMES_IN_FLIGHT];
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = VOE_RENDER_FRAMES_IN_FLIGHT,
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

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		barriers[i] = (VkImageMemoryBarrier2){
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
					 VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_GENERAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = target->images[i].colour.image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		};
	}

	result = voe_render_vk.allocate_command_buffers(device->device,
							&allocate, &commands);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateCommandBuffers failed settling a target (VkResult %d)",
			       (int)result);
		return false;
	}

	voe_render_vk.begin_command_buffer(commands, &begin);
	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
	voe_render_vk.end_command_buffer(commands);

	submit_commands.commandBuffer = commands;
	result = voe_render_vk.queue_submit2(device->queue, 1, &submit,
					     VK_NULL_HANDLE);
	if (result == VK_SUCCESS)
		voe_render_vk.device_wait_idle(device->device);
	else
		VOE_BASE_ERROR("render",
			       "vkQueueSubmit2 failed settling a target (VkResult %d)",
			       (int)result);

	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
	return result == VK_SUCCESS;
}

// Every frame slot's pair for one target, at `extent`, settled. On failure
// nothing is left behind and the target's extent is nought.
static bool build_target_images(voe_render_device *device,
				struct voe_render_target_slot *target,
				VkExtent2D extent)
{
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		// SAMPLED as well as COLOUR_ATTACHMENT, because showing its
		// picture is what a target is for, and TRANSFER_SRC because
		// voe_render_target_read copies that picture into memory.
		// Nothing blits a target anywhere; the copy is the only
		// transfer it is ever the source of.
		if (!build_image(device, &target->images[i].colour, extent,
				 device->format.format,
				 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
					 VK_IMAGE_USAGE_SAMPLED_BIT |
					 VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				 VK_IMAGE_ASPECT_COLOR_BIT, "caller's colour") ||
		    !build_image(device, &target->images[i].depth, extent,
				 VOE_RENDER_DEPTH_FORMAT,
				 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				 VK_IMAGE_ASPECT_DEPTH_BIT, "caller's depth")) {
			teardown_target_images(device, target);
			return false;
		}
	}

	if (!settle(device, target)) {
		teardown_target_images(device, target);
		return false;
	}

	target->extent = extent;
	target->wanted = extent;
	return true;
}

bool voe_render_target_create(voe_render_device *device, uint32_t width,
			      uint32_t height, voe_render_target *out_target,
			      voe_render_texture *out_texture,
			      voe_base_error *error)
{
	struct voe_render_target_slot *target = NULL;
	struct voe_render_texture_slot *texture = NULL;
	uint32_t target_index = 0;
	uint32_t texture_index = 0;

	VOE_BASE_ASSERT(device != NULL, "making a target on no device");
	VOE_BASE_ASSERT(out_target != NULL && out_texture != NULL,
			"making a target with nowhere to hand its ids back");
	VOE_BASE_ASSERT(width > 0 && height > 0,
			"making a target with no pixels in it");
	VOE_BASE_ASSERT(!device->recording,
			"making a target inside an open frame — it rewrites descriptor sets the frame has bound, so it is a startup operation");

	for (uint32_t i = 0; i < device->capacities.targets; i++) {
		if (!device->targets[i].live) {
			target_index = i;
			target = &device->targets[i];
			break;
		}
	}
	if (target == NULL) {
		if (device->capacities.targets == 0)
			VOE_BASE_ERROR("render",
				       "this device was made with room for no targets; `targets` is nought");
		else
			VOE_BASE_ERROR("render",
				       "all %u targets are taken; the device was made with room for that many",
				       device->capacities.targets);
		goto refused;
	}

	// The same search voe_render_texture_create makes, from 1 because slot 0
	// is the white default.
	for (uint32_t i = 1; i < VOE_RENDER_MAX_TEXTURES; i++) {
		if (!device->textures[i].live) {
			texture_index = i;
			texture = &device->textures[i];
			break;
		}
	}
	if (texture == NULL) {
		VOE_BASE_ERROR("render",
			       "all %d texture slots are taken, and a target needs one to be shown through",
			       VOE_RENDER_MAX_TEXTURES);
		goto refused;
	}

	if (!build_target_images(device, target,
				 (VkExtent2D){ width, height }))
		goto refused;

	target->texture = texture_index;
	target->generation++;
	target->live = true;
	target->cleared = false;

	// NEAREST like every picture, and CLAMP_TO_EDGE because a view is not
	// tiled: a coordinate on the far edge of the rectangle showing it would
	// otherwise wrap and read a texel from the opposite side.
	texture->sampling = VOE_RENDER_SAMPLING_SHARP;
	texture->is_target = true;
	texture->target = target_index;
	texture->generation++;
	texture->live = true;

	// settle() has idled the card, so no frame is reading the sets.
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
		voe_render_texture_write_descriptors(device, i);

	out_target->index = target_index + 1;
	out_target->generation = target->generation;
	out_texture->index = texture_index;
	out_texture->generation = texture->generation;
	return true;

refused:
	if (error != NULL)
		*error = VOE_BASE_ERROR_REFUSED;
	return false;
}

void voe_render_target_resize(voe_render_device *device,
			      voe_render_target target, uint32_t width,
			      uint32_t height)
{
	struct voe_render_target_slot *slot;

	VOE_BASE_ASSERT(device != NULL, "resizing a target on no device");
	VOE_BASE_ASSERT(width > 0 && height > 0,
			"resizing a target to no pixels at all");

	slot = voe_render_target_at(device, target);
	VOE_BASE_ASSERT(slot != NULL,
			"resizing a target id that names no target — the window's is resized by the size handed to voe_render_frame_begin");

	// Recorded, not applied: see voe_render_targets_apply_resizes. The size
	// it already has makes `wanted` equal `extent`, which is nothing to do.
	slot->wanted = (VkExtent2D){ width, height };
}

bool voe_render_targets_apply_resizes(voe_render_device *device)
{
	bool idle = false;
	bool changed = false;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "resizing targets on no device");
	VOE_BASE_DEBUG_ASSERT(!device->recording,
			      "resizing targets inside an open frame");

	for (uint32_t i = 0; i < device->capacities.targets; i++) {
		struct voe_render_target_slot *target = &device->targets[i];
		VkExtent2D wanted = target->wanted;

		if (!target->live ||
		    (target->wanted.width == target->extent.width &&
		     target->wanted.height == target->extent.height))
			continue;

		// Once, before the first image goes: a frame in flight on any
		// slot may still be reading any of them.
		if (!idle) {
			voe_render_vk.device_wait_idle(device->device);
			idle = true;
		}

		teardown_target_images(device, target);
		if (!build_target_images(device, target, wanted)) {
			// Still wanted, so a later frame asks the card again.
			target->wanted = wanted;
			return false;
		}
		changed = true;
	}

	if (changed)
		for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
			voe_render_texture_write_descriptors(device, i);
	return true;
}

// ----------------------------------------------- reading a picture back

// One channel, premultiplied by alpha, divided back out. Rounded rather than
// truncated, so that an opaque pixel comes back as the byte that went in, and
// clamped because a blend is allowed to leave a channel above its own alpha.
static uint8_t straight(unsigned value, unsigned alpha)
{
	unsigned out = (value * 255u + alpha / 2u) / alpha;

	return (uint8_t)(out > 255u ? 255u : out);
}

// One target's picture, in its own byte order and premultiplied, turned into
// what a caller was promised: RGBA8 with straight alpha.
//
// THE DIVIDE IS ON THE ENCODED BYTES AND THAT IS DELIBERATE. The format carries
// the sRGB curve, so these bytes are encoded; un-premultiplying them exactly
// would mean decoding, dividing and encoding again, and nothing in this engine
// draws a target whose alpha is anything but one — the clear colour is opaque,
// so every pixel of every picture taken so far divides by 255/255. Where a
// caller does compose a target out of transparent pixels, this is the cheap
// answer, and voe_render_target_read's header says which one it is.
//
// An alpha of nought has no colour to recover, so the pixel comes back as
// transparent black rather than as a division by nothing.
static void into_rgba(uint8_t *out, const uint8_t *in, size_t pixels, bool bgra)
{
	const size_t red = bgra ? 2 : 0;
	const size_t blue = bgra ? 0 : 2;

	for (size_t i = 0; i < pixels; i++) {
		const uint8_t *from = in + i * 4;
		uint8_t *to = out + i * 4;
		unsigned alpha = from[3];

		if (alpha == 0) {
			to[0] = 0;
			to[1] = 0;
			to[2] = 0;
			to[3] = 0;
			continue;
		}

		to[0] = straight(from[red], alpha);
		to[1] = straight(from[1], alpha);
		to[2] = straight(from[blue], alpha);
		to[3] = (uint8_t)alpha;
	}
}

// The copy itself: one command buffer, recorded, submitted and waited for. The
// same one-shot shape settle() above has, and the same reason for waiting — the
// picture is wanted now, by a caller who has just ended a frame.
//
// `transition` IS FOR THE ONE CASE THE LAYOUTS DO NOT COVER. A window target
// that no frame has ended into is still UNDEFINED, which vkCmdCopyImageToBuffer
// does not accept as a source layout; a barrier out of UNDEFINED is always legal
// and discards only what was already undefined. Every other case is already in a
// layout the copy takes: TRANSFER_SRC_OPTIMAL for the window after a frame,
// GENERAL for a target of one's own always.
static bool copy_out(voe_render_device *device, VkImage image,
		     VkImageLayout layout, bool transition, VkExtent2D extent,
		     VkBuffer buffer)
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
	VkImageMemoryBarrier2 readable = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	VkDependencyInfo into_readable = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &readable,
	};
	VkBufferImageCopy region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.imageExtent = { extent.width, extent.height, 1 },
	};
	// What makes the copy visible to the map below. The idle wait after the
	// submit is documented to do the same job, and saying it here as well
	// costs one barrier and removes the question — the same pair
	// render/tests/offscreen.c uses.
	VkMemoryBarrier2 visible = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
		.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
	};
	VkDependencyInfo into_host = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &visible,
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

	// The frame that drew this picture may still be on the card, and this
	// copy has nothing to order itself against it with — the frame's fence
	// belongs to frame.c. Idle is what says the picture is finished.
	voe_render_vk.device_wait_idle(device->device);

	result = voe_render_vk.allocate_command_buffers(device->device,
							&allocate, &commands);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateCommandBuffers failed reading a target back (VkResult %d)",
			       (int)result);
		return false;
	}

	voe_render_vk.begin_command_buffer(commands, &begin);
	if (transition)
		voe_render_vk.cmd_pipeline_barrier2(commands, &into_readable);
	voe_render_vk.cmd_copy_image_to_buffer(commands, image, layout, buffer,
					       1, &region);
	voe_render_vk.cmd_pipeline_barrier2(commands, &into_host);
	voe_render_vk.end_command_buffer(commands);

	submit_commands.commandBuffer = commands;
	result = voe_render_vk.queue_submit2(device->queue, 1, &submit,
					     VK_NULL_HANDLE);
	if (result == VK_SUCCESS)
		voe_render_vk.device_wait_idle(device->device);
	else
		VOE_BASE_ERROR("render",
			       "vkQueueSubmit2 failed reading a target back (VkResult %d)",
			       (int)result);

	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
	return result == VK_SUCCESS;
}

bool voe_render_target_read(voe_render_device *device, voe_render_target target,
			    voe_base_arena *arena, voe_render_picture *out,
			    voe_base_error *error)
{
	const struct voe_render_allocated_image *colour;
	struct voe_render_buffer staging = { 0 };
	struct voe_render_target_slot *own;
	VkExtent2D extent;
	VkImageLayout layout;
	bool transition = false;
	uint32_t slot;
	VkDeviceSize bytes;
	void *mapped = NULL;
	VkResult result;

	VOE_BASE_ASSERT(device != NULL, "reading a target back off no device");
	VOE_BASE_ASSERT(arena != NULL, "reading a target back into no arena");
	VOE_BASE_ASSERT(out != NULL,
			"reading a target back with nowhere to describe it");
	VOE_BASE_ASSERT(!device->recording,
			"reading a target back inside an open frame — it waits for the card, so it belongs between frames beside voe_render_target_create");

	// The frame that ended last drew into the slot before the one the next
	// frame will use: voe_render_frame_end spends a slot the moment its
	// submit lands.
	slot = (device->slot + VOE_RENDER_FRAMES_IN_FLIGHT - 1) %
	       VOE_RENDER_FRAMES_IN_FLIGHT;

	if (target.index == VOE_RENDER_TARGET_WINDOW.index) {
		colour = &device->frames[slot].target.colour;
		extent = device->resolution;
		layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		transition = !device->frame_ended;
	} else {
		own = voe_render_target_at(device, target);
		VOE_BASE_ASSERT(own != NULL,
				"reading back a target id that names no target");
		colour = &own->images[slot].colour;
		extent = own->extent;
		// A target of one's own rests in GENERAL for its whole life,
		// drawn into or not; see the top of this file.
		layout = VK_IMAGE_LAYOUT_GENERAL;
	}

	if (extent.width == 0 || extent.height == 0 ||
	    colour->image == VK_NULL_HANDLE) {
		VOE_BASE_ERROR("render",
			       "this target has no pixels to read — a window with no area has no picture");
		goto refused;
	}

	bytes = (VkDeviceSize)extent.width * extent.height * 4;

	// Coherent as well as visible, so that reading it after the idle wait
	// needs no invalidate call.
	if (!voe_render_buffer_build(device, &staging, bytes,
				     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		goto refused;

	if (!copy_out(device, colour->image, layout, transition, extent,
		      staging.buffer))
		goto refused_with_buffer;

	result = voe_render_vk.map_memory(device->device, staging.memory, 0,
					  VK_WHOLE_SIZE, 0, &mapped);
	if (result != VK_SUCCESS || mapped == NULL) {
		VOE_BASE_ERROR("render",
			       "vkMapMemory failed on the buffer a target was read into (VkResult %d)",
			       (int)result);
		goto refused_with_buffer;
	}

	out->width = extent.width;
	out->height = extent.height;
	out->pixels = voe_base_arena_push(arena, (size_t)bytes);
	into_rgba(out->pixels, mapped, (size_t)extent.width * extent.height,
		  device->format.format == VK_FORMAT_B8G8R8A8_SRGB);

	voe_render_vk.unmap_memory(device->device, staging.memory);
	voe_render_buffer_teardown(device, &staging);
	return true;

refused_with_buffer:
	voe_render_buffer_teardown(device, &staging);
refused:
	if (error != NULL)
		*error = VOE_BASE_ERROR_REFUSED;
	return false;
}
