// What the relight's building (bounce_relight_build.c) and its recording
// (bounce_relight.c) share: the set layout's bindings, the push block member
// for member with bounce_relight.slang's, how many volumes a device relights,
// and the bit of a list word that says a probe holds a picture. Included by
// those two files only; the rest of render reaches the relight through
// device_calls.h.
#ifndef VOE_RENDER_BOUNCE_RELIGHT_H
#define VOE_RENDER_BOUNCE_RELIGHT_H

#include "device_internal.h"

#define VOE_RENDER_RELIGHT_BINDINGS 12
#define VOE_RENDER_RELIGHT_HOLDS 0x80000000u
// Seven grids of three images: six levels, then the sum.
#define VOE_RENDER_RELIGHT_LEVELS 6
#define VOE_RENDER_RELIGHT_GRID_IMAGES (3 * (VOE_RENDER_RELIGHT_LEVELS + 1))
#define VOE_RENDER_RELIGHT_LEVEL_IMAGES (3 * VOE_RENDER_RELIGHT_LEVELS)

// bounce_relight.slang's push block, member for member: settle reads the first
// two, relight the rest.
struct voe_render_relight_push {
	uint32_t first;
	uint32_t count;
	uint32_t chain;
	uint32_t level;
	uint32_t sun_chain;
	uint32_t lamps;
	uint32_t point_ready;
	uint32_t reserved;
};

static const VkDescriptorType
	voe_render_relight_binding_types[VOE_RENDER_RELIGHT_BINDINGS] = {
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	};

static const uint32_t
	voe_render_relight_binding_counts[VOE_RENDER_RELIGHT_BINDINGS] = {
		1, 1, 1, 1, VOE_RENDER_RELIGHT_GRID_IMAGES,
		VOE_RENDER_RELIGHT_LEVEL_IMAGES, 1, 1, 1, 1, 1, 1,
	};

// The volumes a device relights, a set, a list band and a record region each,
// at its descriptor index (voe_render_bounce_volume_index): every volume of the
// window and of each target.
static inline uint32_t
voe_render_relight_volume_count(const voe_render_device *device)
{
	return (device->capacities.targets + 1) * VOE_RENDER_BOUNCE_VOLUMES;
}

#endif
