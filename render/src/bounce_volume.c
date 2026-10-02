// A target's probe volume (ADR-0326 points 2 and 8): its images built on first
// use and freed when unused, and voe_render_bounce_begin, which records a
// target's bounce for the frame.
//
// WHAT IT OWNS. Per volume, the three atlases and the 22 3D images of struct
// voe_render_bounce_volume: the validity, 24 × 12 × 24, and the 21 grid images
// of six-axis irradiance (ADR-0327), twice as wide, a probe's + and − texels
// side by side along x. Every one storage, sampled and transfer-dst (and
// -src, which a test reads one back through), cleared to nought and resting in
// GENERAL; card 04's probe bookkeeping lives beside
// them. The albedo atlas is sRGB, which no card stores to, so it is made
// mutable with extended usage and viewed sampled only. The window's volume is
// the device's, each target's its slot's; device.c tears them down. A build
// names it at bindings 6 and 10, the window's as volume 0, target n's as n.
//
// THE LIFETIME. A begin wants the volume and zeroes its idle count; the top of
// the next frame builds it, and from then each frame top counts one idle frame.
// Past VOE_RENDER_BOUNCE_IDLE of them it is freed and wants nothing until the
// next begin. A begin onto an unbuilt volume bounces nothing that frame; onto a
// built one it places the probes, queuing what card 04 says to capture.
//
// WHY THE TOP OF A FRAME. Building or freeing images wants a card that reads
// none of them, and the top of a frame is where nothing is recording: the same
// place, and the same one idle, as a target's resize. A frame that builds or
// frees nothing does not wait.
//
// CONSTRAINTS. One allocation per image, as target.c makes; 25 a volume. The
// apply walks every target each frame, a few compares each.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <string.h>

// Atlas texels: a probe is six faces across and one face down.
#define ATLAS_WIDTH                                                            \
	(6u * VOE_RENDER_BOUNCE_FACE * VOE_RENDER_BOUNCE_PROBES_XZ)
#define ATLAS_HEIGHT                                                           \
	(VOE_RENDER_BOUNCE_FACE * VOE_RENDER_BOUNCE_PROBES_Y *                 \
	 VOE_RENDER_BOUNCE_PROBES_XZ)
// The atlases, the validity and 7 × 3 grid images, one an axis.
#define VOLUME_IMAGES (4u + 7u * 3u)

// The kinds of image list_images holds.
enum image_kind { ATLAS, VALIDITY, GRID };

static_assert(ATLAS_WIDTH == 1152 && ATLAS_HEIGHT == 2304,
	      "the atlas ADR-0326 point 3 lays out");

// One image: 2D at the atlas size, or 3D a texel a probe (the validity) or two
// along x (a grid), `format`, with `flags`; `view_usage` non-zero narrows the
// view's usage to it. Messages name `what`.
static bool build_image(voe_render_device *device,
			struct voe_render_allocated_image *out,
			enum image_kind kind, VkFormat format,
			VkImageCreateFlags flags, VkImageUsageFlags view_usage,
			const char *what)
{
	const bool atlas = kind == ATLAS;
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags = flags,
		.imageType = atlas ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D,
		.format = format,
		.extent = atlas ? (VkExtent3D){ ATLAS_WIDTH, ATLAS_HEIGHT, 1 }
				: (VkExtent3D){ (kind == GRID ? 2u : 1u) *
							VOE_RENDER_BOUNCE_PROBES_XZ,
						VOE_RENDER_BOUNCE_PROBES_Y,
						VOE_RENDER_BOUNCE_PROBES_XZ },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
			 VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			 VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};
	VkImageViewUsageCreateInfo narrowed = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO,
		.usage = view_usage,
	};
	VkImageViewCreateInfo view = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = view_usage != 0 ? &narrowed : NULL,
		.viewType = atlas ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D,
		.format = format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	};
	VkResult result;

	VOE_BASE_DEBUG_ASSERT(device != NULL && out != NULL,
			      "building a volume image with no device or nowhere");
	result = voe_render_vk.create_image(device->device, &info, NULL,
					    &out->image);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateImage failed for a bounce volume's %s (VkResult %d)",
			       what, (int)result);
		out->image = VK_NULL_HANDLE;
		return false;
	}
	voe_render_vk.get_image_memory_requirements(device->device, out->image,
						    &requirements);
	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = voe_render_memory_type(
		device, requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocate.memoryTypeIndex == UINT32_MAX) {
		VOE_BASE_ERROR("render",
			       "this graphics card offers no device-local memory a bounce volume's %s can live in",
			       what);
		return false;
	}
	result = voe_render_vk.allocate_memory(device->device, &allocate, NULL,
					       &out->memory);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateMemory failed for %llu bytes of a bounce volume's %s (VkResult %d)",
			       (unsigned long long)requirements.size, what,
			       (int)result);
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
			       "binding or viewing a bounce volume's %s failed (VkResult %d)",
			       what, (int)result);
		out->view = VK_NULL_HANDLE;
		return false;
	}
	return true;
}

// Every image of `volume`, in build order: the three atlases, the validity,
// then the grids, image a of a grid axis a.
static void list_images(struct voe_render_bounce_volume *volume,
			struct voe_render_allocated_image **images)
{
	VOE_BASE_DEBUG_ASSERT(volume != NULL && images != NULL,
			      "listing no volume's images");
	images[0] = &volume->albedo;
	images[1] = &volume->normal;
	images[2] = &volume->moments;
	images[3] = &volume->validity;
	for (uint32_t g = 0; g < 7; g++)
		for (uint32_t k = 0; k < 3; k++)
			images[4 + 3 * g + k] = &volume->irradiance[g][k];
}

// Image `i` of list_images built.
static bool build_listed(voe_render_device *device,
			 struct voe_render_allocated_image *image, uint32_t i)
{
	switch (i) {
	case 0:
		return build_image(device, image, ATLAS,
				   VK_FORMAT_R8G8B8A8_SRGB,
				   VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT |
					   VK_IMAGE_CREATE_EXTENDED_USAGE_BIT,
				   VK_IMAGE_USAGE_SAMPLED_BIT |
					   VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				   "albedo atlas");
	case 1:
		return build_image(device, image, ATLAS,
				   VK_FORMAT_R16G16B16A16_SFLOAT, 0, 0,
				   "normal and distance atlas");
	case 2:
		return build_image(device, image, ATLAS, VK_FORMAT_R16G16_SFLOAT,
				   0, 0, "moments atlas");
	case 3:
		return build_image(device, image, VALIDITY, VK_FORMAT_R16_SFLOAT,
				   0, 0, "validity");
	default:
		return build_image(device, image, GRID,
				   VK_FORMAT_R16G16B16A16_SFLOAT, 0, 0,
				   "six-axis irradiance grid");
	}
}

void voe_render_bounce_volume_teardown(voe_render_device *device,
				       struct voe_render_bounce_volume *volume)
{
	struct voe_render_allocated_image *images[VOLUME_IMAGES];

	VOE_BASE_DEBUG_ASSERT(device != NULL && volume != NULL,
			      "tearing down a volume with no device or volume");
	list_images(volume, images);
	for (uint32_t i = 0; i < VOLUME_IMAGES; i++)
		voe_render_target_image_teardown(device, images[i]);
	memset(volume, 0, sizeof(*volume));
}

bool voe_render_bounce_volume_build(voe_render_device *device,
				    struct voe_render_bounce_volume *volume)
{
	struct voe_render_allocated_image *images[VOLUME_IMAGES];
	VkImageMemoryBarrier2 barriers[VOLUME_IMAGES];
	VkImage clears[VOLUME_IMAGES];

	VOE_BASE_DEBUG_ASSERT(device != NULL && volume != NULL,
			      "building a volume with no device or volume");
	VOE_BASE_DEBUG_ASSERT(!volume->built, "building a built volume");

	list_images(volume, images);
	for (uint32_t i = 0; i < VOLUME_IMAGES; i++) {
		if (!build_listed(device, images[i], i)) {
			voe_render_bounce_volume_teardown(device, volume);
			return false;
		}
		clears[i] = images[i]->image;
		barriers[i] = (VkImageMemoryBarrier2){
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_GENERAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = clears[i],
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		};
	}
	if (!voe_render_target_settle_cleared(device, barriers, VOLUME_IMAGES,
					      clears, VOLUME_IMAGES)) {
		voe_render_bounce_volume_teardown(device, volume);
		return false;
	}
	volume->built = true;
	volume->idle = 0;
	return true;
}

// One volume's top of frame: built when wanted and named at volume `index` of
// every set, counted idle when built, and freed past VOE_RENDER_BOUNCE_IDLE.
// `idle` says whether the card is idle yet.
static bool apply_one(voe_render_device *device,
		      struct voe_render_bounce_volume *volume, uint32_t index,
		      bool *idle)
{
	VOE_BASE_DEBUG_ASSERT(volume != NULL && idle != NULL,
			      "applying no volume");
	if (volume->wanted && !volume->built) {
		if (!voe_render_bounce_volume_build(device, volume))
			return false;
		voe_render_descriptors_write_volume(device, volume, index);
		*idle = true;
	}
	if (volume->built && ++volume->idle > VOE_RENDER_BOUNCE_IDLE) {
		// Once, before the first image goes: a frame in flight may
		// still be reading it.
		if (!*idle) {
			voe_render_vk.device_wait_idle(device->device);
			*idle = true;
		}
		voe_render_bounce_volume_teardown(device, volume);
	}
	return true;
}

bool voe_render_bounce_volumes_apply(voe_render_device *device)
{
	bool idle = false;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "applying volumes on no device");
	VOE_BASE_DEBUG_ASSERT(!device->recording,
			      "applying volumes inside an open frame");
	if (!apply_one(device, &device->window_volume, 0, &idle))
		return false;
	for (uint32_t i = 0; i < device->capacities.targets; i++)
		if (device->targets[i].live &&
		    !apply_one(device, &device->targets[i].volume, i + 1, &idle))
			return false;
	return true;
}

struct voe_render_bounce_volume *
voe_render_bounce_volume_of(voe_render_device *device, voe_render_target target)
{
	struct voe_render_target_slot *own;

	if (target.index == VOE_RENDER_TARGET_WINDOW.index &&
	    target.generation == VOE_RENDER_TARGET_WINDOW.generation)
		return &device->window_volume;
	own = voe_render_target_at(device, target);
	VOE_BASE_ASSERT(own != NULL,
			"a bounce begin for a target id that names no live target");
	return &own->volume;
}

void voe_render_bounce_begin(voe_render_device *device, voe_render_target target,
			     const struct voe_render_bounce_frame *frame)
{
	struct voe_render_bounce_volume *volume;
	struct voe_render_bounce_begun *begun;
	voe_render_bounce_lights lights;

	VOE_BASE_ASSERT(device != NULL && frame != NULL,
			"a bounce begin with no device or no frame record");
	VOE_BASE_ASSERT(frame->stale_count == 0 || frame->stale != NULL,
			"a bounce begin with stale spheres and no array");
	VOE_BASE_ASSERT(frame->points.count == 0 || frame->points.lights != NULL,
			"a bounce begin with point lights and no array");
	VOE_BASE_ASSERT(device->recording, "a bounce begin with no frame open");
	VOE_BASE_ASSERT(!device->pass_open,
			"a bounce begin inside a pass — it records between passes");
	VOE_BASE_ASSERT(frame->sun_bounces <= VOE_RENDER_BOUNCES_MAX,
			"a bounce begin whose sun bounces past VOE_RENDER_BOUNCES_MAX");

	volume = voe_render_bounce_volume_of(device, target);
	begun = &volume->begun[device->slot];
	VOE_BASE_ASSERT(!begun->begun,
			"a second bounce begin for one target in one frame");
	begun->begun = true;
	memcpy(begun->cell, frame->cell, sizeof(begun->cell));
	begun->corner[0] = frame->corner.x;
	begun->corner[1] = frame->corner.y;
	begun->corner[2] = frame->corner.z;
	if (!device->output_layer)
		return;

	volume->wanted = true;
	volume->idle = 0;
	device->bounce_begun = true;
	device->bounce_target = target;
	device->bounce_frame = *frame;
	device->bounce_frame.stale = NULL;
	device->bounce_frame.stale_count = 0;
	device->bounce_frame.points =
		(voe_render_point_lights){ .lights = device->bounce_lamps };
	if (!volume->built)
		return;

	voe_render_bounce_probes_place(&volume->probes, frame->cell,
				       frame->corner, frame->stale,
				       frame->stale_count, &frame->sun,
				       frame->sun_bounces, frame->sun_strength,
				       &frame->points, &lights);
	memcpy(device->bounce_lamps, lights.lamps, sizeof(device->bounce_lamps));
	device->bounce_frame.points.count = lights.lamp_count;
}
