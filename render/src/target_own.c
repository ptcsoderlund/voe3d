// The targets of a caller's own (ADR-0148): the window's pair from target.c made
// again, one colour and one depth image per frame slot, in the window's formats,
// at a size of their own. What differs is that the colour image is also sampled
// — its picture is shown through one ordinary texture slot — and that is the
// whole of what this file is about. The table of them is made at startup, a
// resize is recorded and applied at the top of a frame, and every image goes
// back at shutdown.
//
// ONE TEXTURE SLOT, AND EACH FRAME SLOT'S DESCRIPTOR SET POINTS IT AT THAT FRAME
// SLOT'S IMAGE. There is already a set per frame slot, each holding the whole
// texture table, so no new binding, set or shader change was needed: the
// descriptor write reads the frame slot it is writing and names that slot's
// image for a target's texture. See voe_render_texture_write_descriptors. A
// second slot shows the depth copy beside each depth image the same way
// (ADR-0305), so a target costs two.
//
// The images themselves are built and torn down by target.c's
// voe_render_target_image_build and _teardown, so a caller's target and the
// window's are one copy of that code and not two.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdlib.h>

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
		voe_render_target_image_teardown(device,
						 &target->images[i].depth_copy);
		voe_render_target_image_teardown(device,
						 &target->images[i].depth);
		voe_render_target_image_teardown(device,
						 &target->images[i].colour);
	}
	target->extent = (VkExtent2D){ 0, 0 };
}

void voe_render_targets_shutdown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "shutting targets down on no device");

	// The caller of this has already waited for the card; see close_down in
	// device.c.
	if (device->targets == NULL)
		return;

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
// The depth copies settle in the same submit, into SHADER_READ_ONLY_OPTIMAL.
static bool settle(voe_render_device *device,
		   const struct voe_render_target_slot *target)
{
	VkImageMemoryBarrier2 barriers[2 * VOE_RENDER_FRAMES_IN_FLIGHT];

	VOE_BASE_DEBUG_ASSERT(device != NULL, "settling a target on no device");
	VOE_BASE_DEBUG_ASSERT(target != NULL, "settling no target");

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		barriers[VOE_RENDER_FRAMES_IN_FLIGHT + i] =
			voe_render_target_settle_copy(
				target->images[i].depth_copy.image);
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

	return voe_render_target_settle(device, barriers,
					2 * VOE_RENDER_FRAMES_IN_FLIGHT);
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
		if (!voe_render_target_image_build(
			    device, &target->images[i].colour, extent,
			    device->format.format,
			    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
				    VK_IMAGE_USAGE_SAMPLED_BIT |
				    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			    VK_IMAGE_ASPECT_COLOR_BIT, "caller's colour") ||
		    !voe_render_target_image_build(
			    device, &target->images[i].depth, extent,
			    VOE_RENDER_DEPTH_FORMAT,
			    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
				    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			    VK_IMAGE_ASPECT_DEPTH_BIT, "caller's depth") ||
		    !voe_render_target_depth_copy_build(
			    device, &target->images[i].depth_copy, extent,
			    "caller's depth copy")) {
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

static bool create_target(voe_render_device *device, uint32_t width,
			  uint32_t height, voe_render_target *out_target,
			  voe_render_texture *out_texture, voe_base_error *error)
{
	struct voe_render_target_slot *target = NULL;
	struct voe_render_texture_slot *texture = NULL;
	struct voe_render_texture_slot *depth = NULL;
	uint32_t target_index = 0;
	uint32_t texture_index = 0;
	uint32_t depth_index = 0;

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

	// Two slots: the picture and the depth copy (ADR-0305).
	texture_index = voe_render_target_free_texture(device, 0);
	if (texture_index != 0)
		depth_index = voe_render_target_free_texture(device,
							     texture_index);
	if (depth_index == 0) {
		VOE_BASE_ERROR("render",
			       "all %d texture slots are taken, and a target needs two: its picture and its depth copy",
			       VOE_RENDER_MAX_TEXTURES);
		goto refused;
	}
	texture = &device->textures[texture_index];
	depth = &device->textures[depth_index];

	if (!build_target_images(device, target,
				 (VkExtent2D){ width, height }))
		goto refused;

	target->texture = texture_index;
	target->depth_texture = depth_index;
	target->generation++;
	target->live = true;
	target->cleared = false;

	// NEAREST like every picture, and CLAMP_TO_EDGE because a view is not
	// tiled: a coordinate on the far edge of the rectangle showing it would
	// otherwise wrap and read a texel from the opposite side.
	texture->sampling = VOE_RENDER_SAMPLING_SHARP;
	texture->is_target = true;
	texture->depth = false;
	texture->target = target_index;
	texture->generation++;
	texture->live = true;
	depth->sampling = VOE_RENDER_SAMPLING_SHARP;
	depth->is_target = true;
	depth->depth = true;
	depth->target = target_index;
	depth->generation++;
	depth->live = true;

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

// Under the device's guard (ADR-0370), as is the resize.
bool voe_render_target_create(voe_render_device *device, uint32_t width,
			      uint32_t height, voe_render_target *out_target,
			      voe_render_texture *out_texture,
			      voe_base_error *error)
{
	bool created;

	VOE_BASE_ASSERT(device != NULL, "making a target on no device");

	voe_render_device_guard_take(device);
	created = create_target(device, width, height, out_target, out_texture,
				error);
	voe_render_device_guard_give(device);
	return created;
}

void voe_render_target_resize(voe_render_device *device,
			      voe_render_target target, uint32_t width,
			      uint32_t height)
{
	struct voe_render_target_slot *slot;

	VOE_BASE_ASSERT(device != NULL, "resizing a target on no device");
	VOE_BASE_ASSERT(width > 0 && height > 0,
			"resizing a target to no pixels at all");

	voe_render_device_guard_take(device);
	slot = voe_render_target_at(device, target);
	VOE_BASE_ASSERT(slot != NULL,
			"resizing a target id that names no target — the window's is resized by the size handed to voe_render_frame_begin");

	// Recorded, not applied: see voe_render_targets_apply_resizes. The size
	// it already has makes `wanted` equal `extent`, which is nothing to do.
	slot->wanted = (VkExtent2D){ width, height };
	voe_render_device_guard_give(device);
}

// A RESIZE IS APPLIED AT THE TOP OF A FRAME AND IT WAITS FOR THE CARD. Every
// slot's images are thrown away together, for the reason the window's are: a
// fence says one slot is finished and says nothing about the others.
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
