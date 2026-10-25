// What the material textures take: voe_render_texture_memory_smooth, declared
// in render/include/render/device.h, for the Frame panel (ADR-0477 point 7).
//
// ONE WALK OF THE SLOT TABLE. A material texture is a live SMOOTH slot from 1
// on: slot 0 is the white default, and heights, paint, targets, glyphs and
// sheets are SHARP or FIELD. Each slot's `bytes` is what the card asked for when
// its memory was bound, so a compressed image counts what it really takes.
//
// Constraints: reads the table with no guard, so on the owner's thread, which is
// the one no worker's create or destroy races; 1024 slots a call.
#include "device_internal.h"

#include <base/assert.h>

voe_render_texture_memory
voe_render_texture_memory_smooth(const voe_render_device *device)
{
	voe_render_texture_memory memory = { 0 };

	VOE_BASE_DEBUG_ASSERT(device != NULL, "counting textures on no device");

	for (uint32_t i = 1; i < VOE_RENDER_MAX_TEXTURES; i++) {
		const struct voe_render_texture_slot *slot = &device->textures[i];

		if (!slot->live || slot->is_target || slot->heights ||
		    slot->sampling != VOE_RENDER_SAMPLING_SMOOTH)
			continue;
		memory.bytes += slot->bytes;
		memory.count++;
		if (slot->blocks)
			memory.compressed++;
	}

	VOE_BASE_DEBUG_ASSERT(memory.compressed <= memory.count,
			      "more compressed textures than textures");
	return memory;
}
