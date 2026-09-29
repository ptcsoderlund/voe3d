// The gamepad slots and range conversions of src/gamepad.h, once, for both
// platforms. No OS header and no #ifdef: a backend calls these with what its
// device reported, and nothing here knows which device that was.
//
// The conversions work in double so that a 32-bit range's width cannot
// overflow and its middle is exact.
#include "gamepad.h"

#include <base/assert.h>

#include <string.h>

int voe_platform_gamepad_attach(struct voe_platform_gamepads *pads)
{
	VOE_BASE_DEBUG_ASSERT(pads != NULL, "attaching a pad to nothing");

	for (int slot = 0; slot < VOE_PLATFORM_GAMEPAD_SLOTS; slot++) {
		voe_platform_gamepad *pad = &pads->slots[slot];

		if (pad->connected)
			continue;
		pads->next_id++;
		memset(pad, 0, sizeof(*pad));
		pad->connected = true;
		pad->id = pads->next_id;
		return slot;
	}
	return -1;
}

void voe_platform_gamepad_detach(struct voe_platform_gamepads *pads, int slot)
{
	VOE_BASE_DEBUG_ASSERT(pads != NULL, "detaching a pad from nothing");
	VOE_BASE_DEBUG_ASSERT(slot >= 0 && slot < VOE_PLATFORM_GAMEPAD_SLOTS,
			      "detaching a slot out of range");

	memset(&pads->slots[slot], 0, sizeof(pads->slots[slot]));
}

float voe_platform_gamepad_axis(int32_t value, int32_t min, int32_t max)
{
	double middle;
	double half;
	double result;

	if (max <= min)
		return 0.0f;
	middle = ((double)min + (double)max) / 2.0;
	half = ((double)max - (double)min) / 2.0;
	result = ((double)value - middle) / half;
	if (result < -1.0)
		return -1.0f;
	if (result > 1.0)
		return 1.0f;
	return (float)result;
}

float voe_platform_gamepad_trigger(int32_t value, int32_t min, int32_t max)
{
	double result;

	if (max <= min)
		return 0.0f;
	result = ((double)value - (double)min) / ((double)max - (double)min);
	if (result < 0.0)
		return 0.0f;
	if (result > 1.0)
		return 1.0f;
	return (float)result;
}
