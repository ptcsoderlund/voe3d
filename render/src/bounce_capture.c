// A capture pass (ADR-0326 point 3): up to VOE_RENDER_BOUNCE_CAPTURE probes'
// six-face pictures drawn in one layered pass, ADR-0325's point-shadow drawing
// with colour, then copied into the begun target's albedo and normal atlases.
// voe_render_bounce_capture_pass_begin opens it; voe_render_pass_end calls
// voe_render_bounce_capture_end once its rendering has ended.
//
// THE SCRATCH. Per frame slot an albedo (RGBA8 sRGB), a normal and distance
// (RGBA16F) and a D32 depth image, 6 × VOE_RENDER_BOUNCE_CAPTURE layers of
// VOE_RENDER_BOUNCE_FACE square, each viewed whole for the attachment. Built at
// startup with the device when it has shaderOutputLayer, none without. No resting
// layout: every begin takes them from UNDEFINED, because it clears them.
//
// WHY A SCRATCH AND A COPY. An atlas holds 6912 probes' faces, and a layered pass
// draws into layers: an atlas of 41472 layers is past maxImageArrayLayers on most
// cards, and past what one pass should clear. Sixteen probes are drawn into 96
// layers and each face copied into its tile: probe (i, j, k), its toroidal
// coordinate, at x 48i + 8f, y 8(12k + j).
//
// THE BEGIN takes up to sixteen queued probes nearest the eye (card 04's take)
// and writes them as the pass's point lights by slot, centre about the eye at the
// begun volume's spacing (its block's spacing too), range the volume's reach,
// twelve of its cells (VOE_RENDER_BOUNCE_REACH × spacing / the finest, 0332
// point 4), and into device->pass_casters, so draw.c culls a caster's faces as
// in the point-shadow pass. Colours clear to nought with the normal's w that
// reach, depth to the far plane. THE END moves the scratch to
// TRANSFER_SRC and both atlases from GENERAL to TRANSFER_DST, copies, and moves
// the atlases back to GENERAL.
//
// CONSTRAINTS. At most VOE_RENDER_BOUNCE_CAPTURE_PASSES a frame; a begun target
// whose volume is not built, or whose queue is empty, opens nothing.
#include "frame_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdio.h>

#define LAYERS (6u * VOE_RENDER_BOUNCE_CAPTURE)
#define FACE ((uint32_t)VOE_RENDER_BOUNCE_FACE)
#define XZ ((uint32_t)VOE_RENDER_BOUNCE_PROBES_XZ)
#define H ((uint32_t)VOE_RENDER_BOUNCE_PROBES_Y)

static_assert(VOE_RENDER_BOUNCE_CAPTURE == VOE_RENDER_POINT_SHADOWS,
	      "a capture pass's probes take the point-shadow pass's slots in draw.c");

// One scratch image of every layer, its memory and its 2D-array view, both
// named "bounce capture `what` slot `slot`".
static bool build_image(voe_render_device *device,
			struct voe_render_allocated_image *out, VkFormat format,
			VkImageUsageFlags usage, VkImageAspectFlags aspect,
			const char *what, uint32_t slot)
{
	char name[64];
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = { FACE, FACE, 1 },
		.mipLevels = 1,
		.arrayLayers = LAYERS,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};
	VkImageViewCreateInfo view = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		.format = format,
		.subresourceRange = { .aspectMask = aspect, .levelCount = 1,
				      .layerCount = LAYERS },
	};
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	};
	VkResult result;

	result = voe_render_vk.create_image(device->device, &info, NULL,
					    &out->image);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateImage failed for a bounce capture scratch (VkResult %d)",
			       (int)result);
		out->image = VK_NULL_HANDLE;
		return false;
	}
	snprintf(name, sizeof name, "bounce capture %s slot %u", what, slot);
	voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE,
			      (uint64_t)out->image, name);
	voe_render_vk.get_image_memory_requirements(device->device, out->image,
						    &requirements);
	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = voe_render_memory_type(
		device, requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocate.memoryTypeIndex == UINT32_MAX) {
		VOE_BASE_ERROR("render",
			       "this graphics card offers no device-local memory a bounce capture scratch can live in");
		return false;
	}
	result = voe_render_vk.allocate_memory(device->device, &allocate, NULL,
					       &out->memory);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateMemory failed for %llu bytes of bounce capture scratch (VkResult %d)",
			       (unsigned long long)requirements.size, (int)result);
		out->memory = VK_NULL_HANDLE;
		return false;
	}
	result = voe_render_vk.bind_image_memory(device->device, out->image,
						 out->memory, 0);
	view.image = out->image;
	if (result == VK_SUCCESS)
		result = voe_render_vk.create_image_view(device->device, &view,
							 NULL, &out->view);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "binding or viewing a bounce capture scratch failed (VkResult %d)",
			       (int)result);
		out->view = VK_NULL_HANDLE;
		return false;
	}
	voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE_VIEW,
			      (uint64_t)out->view, name);
	return true;
}

bool voe_render_bounce_capture_startup(voe_render_device *device)
{
	const VkImageUsageFlags colour = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
					 VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VOE_BASE_ASSERT(device != NULL, "making capture scratch on no device");
	if (!device->output_layer)
		return true;
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_bounce_scratch *scratch =
			&device->frames[i].capture;

		if (!build_image(device, &scratch->albedo,
				 VK_FORMAT_R8G8B8A8_SRGB, colour,
				 VK_IMAGE_ASPECT_COLOR_BIT, "albedo", i) ||
		    !build_image(device, &scratch->normal,
				 VK_FORMAT_R16G16B16A16_SFLOAT, colour,
				 VK_IMAGE_ASPECT_COLOR_BIT, "normal", i) ||
		    !build_image(device, &scratch->depth,
				 VOE_RENDER_DEPTH_FORMAT,
				 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				 VK_IMAGE_ASPECT_DEPTH_BIT, "depth", i))
			return false;
	}
	return true;
}

void voe_render_bounce_capture_shutdown(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "taking capture scratch from no device");
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_bounce_scratch *scratch =
			&device->frames[i].capture;

		voe_render_target_image_teardown(device, &scratch->albedo);
		voe_render_target_image_teardown(device, &scratch->normal);
		voe_render_target_image_teardown(device, &scratch->depth);
	}
}

// How far a probe of a volume at `spacing` sees: twelve of its cells, so a
// coarser grid's probes still see past their neighbours (0332 point 4).
static float volume_reach(float spacing)
{
	VOE_BASE_DEBUG_ASSERT(spacing > 0.0f, "the reach of a grid with no spacing");
	return VOE_RENDER_BOUNCE_REACH * spacing / VOE_RENDER_BOUNCE_SPACING;
}

// Whether any probe of `p` is queued for a capture.
static bool any_queued(const voe_render_bounce_probes *p)
{
	for (uint32_t i = 0; i < VOE_RENDER_BOUNCE_PROBES_TOTAL / 32; i++)
		if (p->queued[i] != 0)
			return true;
	return false;
}

// Probe `probe`'s centre about the eye, from where and at the spacing `p` was
// last placed.
static voe_math_float3 probe_centre(const voe_render_bounce_probes *p,
				    uint32_t probe)
{
	const uint32_t at[3] = { probe % XZ, probe / XZ % H, probe / (XZ * H) };
	const uint32_t size[3] = { XZ, H, XZ };
	const float corner[3] = { p->corner.x, p->corner.y, p->corner.z };
	float c[3];

	VOE_BASE_DEBUG_ASSERT(probe < VOE_RENDER_BOUNCE_PROBES_TOTAL,
			      "the centre of a probe the grid does not have");
	for (int a = 0; a < 3; a++) {
		const uint32_t local =
			(at[a] + size[a] -
			 voe_render_bounce_probe_wrap(p->cell[a], size[a])) %
			size[a];

		c[a] = corner[a] + ((float)local + 0.5f) * p->spacing;
	}
	return (voe_math_float3){ c[0], c[1], c[2] };
}

// The `count` probes of `taken` as the open pass's lights by slot, into its
// region of binding 7 and device->pass_casters, and kept for the end's copy.
static void place_probes(voe_render_device *device,
			 const struct voe_render_frame *frame,
			 const voe_render_bounce_probes *p,
			 const uint32_t *taken, uint32_t count)
{
	voe_render_point_light lights[VOE_RENDER_BOUNCE_CAPTURE] = { 0 };

	VOE_BASE_DEBUG_ASSERT(count > 0 && count <= VOE_RENDER_BOUNCE_CAPTURE,
			      "a capture pass with no probes or too many");
	device->pass_slots = 0;
	for (uint32_t i = 0; i < count; i++) {
		lights[i].position = probe_centre(p, taken[i]);
		lights[i].range = volume_reach(p->spacing);
		lights[i].falloff = 1.0f;
		device->pass_casters[i] = lights[i];
		device->capture_probes[i] = taken[i];
		device->pass_slots |= 1u << i;
	}
	device->capture_count = count;
	voe_render_pass_copy_lights(frame, device->pass_count, 0, lights, count);
}

// The scratch from UNDEFINED into its attachment layouts, after any earlier
// copy out of it, and the cleared rendering of every layer begun, the normal's
// w cleared to `reach`.
static void record_open(const struct voe_render_frame *frame, float reach)
{
	const VkPipelineStageFlags2 tests =
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
		VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	const struct voe_render_bounce_scratch *scratch = &frame->capture;
	VkImageMemoryBarrier2 barriers[3];
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 3,
		.pImageMemoryBarriers = barriers,
	};
	VkRenderingAttachmentInfo colours[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = scratch->albedo.view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		},
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = scratch->normal.view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = { .color = { .float32 = {
				0.0f, 0.0f, 0.0f, reach } } },
		},
	};
	VkRenderingAttachmentInfo depth = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = scratch->depth.view,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.clearValue = { .depthStencil = { .depth = VOE_RENDER_DEPTH_CLEAR } },
	};
	const VkExtent2D extent = { FACE, FACE };
	VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .extent = extent },
		.layerCount = LAYERS,
		.colorAttachmentCount = 2,
		.pColorAttachments = colours,
		.pDepthAttachment = &depth,
	};
	VkRect2D scissor = { .extent = extent };
	VkViewport viewport = voe_render_frame_viewport(extent);

	for (int i = 0; i < 3; i++) {
		const bool colour = i < 2;

		barriers[i] = (VkImageMemoryBarrier2){
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = colour ? VK_PIPELINE_STAGE_2_COPY_BIT : tests,
			.srcAccessMask = colour ? VK_ACCESS_2_NONE :
					 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstStageMask = colour ?
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT : tests,
			.dstAccessMask = colour ?
				VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT :
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
					VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = colour ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
				     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = i == 0 ? scratch->albedo.image :
				 i == 1 ? scratch->normal.image :
					  scratch->depth.image,
			.subresourceRange = {
				.aspectMask = colour ? VK_IMAGE_ASPECT_COLOR_BIT :
						       VK_IMAGE_ASPECT_DEPTH_BIT,
				.levelCount = 1,
				.layerCount = LAYERS,
			},
		};
	}
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
	voe_render_vk.cmd_begin_rendering(frame->commands, &rendering);
	voe_render_vk.cmd_set_viewport(frame->commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(frame->commands, 0, 1, &scissor);
}

bool voe_render_bounce_capture_pass_begin(voe_render_device *device,
					  bool *opened)
{
	struct voe_render_bounce_volume *volume;
	struct voe_render_frame *frame;
	struct voe_render_frame_block block = { 0 };
	uint32_t taken[VOE_RENDER_BOUNCE_CAPTURE];
	uint32_t count;
	char name[VOE_RENDER_PASS_NAME];

	VOE_BASE_ASSERT(device != NULL && opened != NULL,
			"opening a capture pass on no device or with nowhere to say so");
	VOE_BASE_ASSERT(device->recording,
			"opening a capture pass with no frame open");
	VOE_BASE_ASSERT(!device->pass_open,
			"opening a capture pass while a pass is already open — passes do not nest");
	*opened = false;
	// Before the begin's assert: a failed prepare also refused the begin.
	if (!voe_render_device_ready(device))
		return false;
	// Without shaderOutputLayer a begin records nothing to assert on.
	VOE_BASE_ASSERT(device->bounce_begun || !device->output_layer,
			"opening a capture pass with no voe_render_bounce_begin this frame");
	if (!device->bounce_begun ||
	    device->capture_passes >= VOE_RENDER_BOUNCE_CAPTURE_PASSES)
		return true;
	volume = voe_render_bounce_volume_of(device, device->bounce_target);
	if (!volume->built || !any_queued(&volume->probes))
		return true;

	if (device->pass_count >= device->capacities.passes) {
		VOE_BASE_ERROR("render",
			       "this frame has already opened %u of %u passes, so a capture pass does not fit; `passes` is too small for what this frame draws",
			       device->pass_count, device->capacities.passes);
		return false;
	}

	count = voe_render_bounce_probes_take(&volume->probes, taken,
					      VOE_RENDER_BOUNCE_CAPTURE);
	frame = voe_render_frame_at(device, device->slot);
	place_probes(device, frame, &volume->probes, taken, count);
	block.depth_copy = VOE_RENDER_NO_DEPTH_COPY;
	block.bounce.grid = VOE_RENDER_NO_BOUNCE;
	block.bounce.spacing = volume->probes.spacing;
	block.region = device->pass_count;
	block.lights = count;

	record_open(frame, volume_reach(volume->probes.spacing));
	device->pass_target = NULL;
	device->pass_extent = (VkExtent2D){ FACE, FACE };
	// Numbered from 1 in the frame (ADR-0367 point 1).
	(void)snprintf(name, sizeof(name), "bounce capture %u",
		       device->capture_passes + 1);
	voe_render_pass_start(device, frame, &block, device->pipeline_capture,
			      name);
	device->pass_camera = true;
	device->pass_shadow = false;
	device->pass_capture = true;
	device->capture_volume = volume;
	device->capture_passes++;
	*opened = true;
	return true;
}

// `image` between `from` and `to` over `layers` layers, after `src` and before
// `dst` stages with their accesses.
static VkImageMemoryBarrier2 move(VkImage image, uint32_t layers,
				  VkImageLayout from, VkImageLayout to,
				  VkPipelineStageFlags2 src, VkAccessFlags2 src_access,
				  VkPipelineStageFlags2 dst, VkAccessFlags2 dst_access)
{
	return (VkImageMemoryBarrier2){
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = src,
		.srcAccessMask = src_access,
		.dstStageMask = dst,
		.dstAccessMask = dst_access,
		.oldLayout = from,
		.newLayout = to,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				      .levelCount = 1,
				      .layerCount = layers },
	};
}

void voe_render_bounce_capture_end(voe_render_device *device)
{
	const VkPipelineStageFlags2 all = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	const VkPipelineStageFlags2 copy = VK_PIPELINE_STAGE_2_COPY_BIT;
	const VkPipelineStageFlags2 output =
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	const VkImageLayout dst = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	struct voe_render_frame *frame;
	struct voe_render_bounce_volume *volume;
	const struct voe_render_bounce_scratch *scratch;
	VkImageMemoryBarrier2 barriers[4];
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 4,
		.pImageMemoryBarriers = barriers,
	};
	VkImageCopy regions[LAYERS];

	VOE_BASE_ASSERT(device != NULL && device->pass_capture,
			"ending a capture outside a capture pass");
	frame = voe_render_frame_at(device, device->slot);
	volume = device->capture_volume;
	scratch = &frame->capture;
	VOE_BASE_DEBUG_ASSERT(volume != NULL && volume->built,
			      "copying a capture into a volume that is not built");

	barriers[0] = move(scratch->albedo.image, LAYERS,
			   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, output,
			   VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, copy,
			   VK_ACCESS_2_TRANSFER_READ_BIT);
	barriers[1] = barriers[0];
	barriers[1].image = scratch->normal.image;
	barriers[2] = move(volume->albedo.image, 1, VK_IMAGE_LAYOUT_GENERAL, dst,
			   all, VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
			   copy, VK_ACCESS_2_TRANSFER_WRITE_BIT);
	barriers[3] = barriers[2];
	barriers[3].image = volume->normal.image;
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);

	for (uint32_t i = 0; i < device->capture_count; i++) {
		const uint32_t probe = device->capture_probes[i];
		const uint32_t x = probe % XZ;
		const uint32_t y = probe / XZ % H;
		const uint32_t z = probe / (XZ * H);

		for (uint32_t f = 0; f < 6; f++)
			regions[6 * i + f] = (VkImageCopy){
				.srcSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						    .baseArrayLayer = 6 * i + f,
						    .layerCount = 1 },
				.dstSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						    .layerCount = 1 },
				.dstOffset = { (int32_t)(FACE * (6 * x + f)),
					       (int32_t)(FACE * (H * z + y)), 0 },
				.extent = { FACE, FACE, 1 },
			};
	}
	voe_render_vk.cmd_copy_image(frame->commands, scratch->albedo.image,
				     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				     volume->albedo.image, dst,
				     6 * device->capture_count, regions);
	voe_render_vk.cmd_copy_image(frame->commands, scratch->normal.image,
				     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				     volume->normal.image, dst,
				     6 * device->capture_count, regions);

	barriers[0] = move(volume->albedo.image, 1, dst, VK_IMAGE_LAYOUT_GENERAL,
			   copy, VK_ACCESS_2_TRANSFER_WRITE_BIT, all,
			   VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT);
	barriers[1] = barriers[0];
	barriers[1].image = volume->normal.image;
	dependency.imageMemoryBarrierCount = 2;
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
	device->capture_volume = NULL;
	device->capture_count = 0;
}
