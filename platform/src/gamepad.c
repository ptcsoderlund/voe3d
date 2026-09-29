// The gamepad slots and range conversions of src/gamepad.h, once, for both
// platforms. No OS header and no #ifdef: a backend calls these with what its
// device reported, and nothing here knows which device that was.
//
// The conversions work in double so that a 32-bit range's width cannot
// overflow and its middle is exact. The evdev translation is a table of the
// kernel's button codes and a switch over its axis codes; the numbers carry
// the kernel's names beside them because linux/input.h is not included.
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

// EV_KEY codes, the kernel's names beside them. North and west are the
// kernel's BTN_X and BTN_Y.
static const struct {
	uint16_t code;
	voe_platform_gamepad_button button;
} evdev_buttons[] = {
	{ 0x130, VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH },	       // BTN_SOUTH
	{ 0x131, VOE_PLATFORM_GAMEPAD_BUTTON_EAST },	       // BTN_EAST
	{ 0x133, VOE_PLATFORM_GAMEPAD_BUTTON_NORTH },	       // BTN_NORTH
	{ 0x134, VOE_PLATFORM_GAMEPAD_BUTTON_WEST },	       // BTN_WEST
	{ 0x136, VOE_PLATFORM_GAMEPAD_BUTTON_LEFT_SHOULDER },  // BTN_TL
	{ 0x137, VOE_PLATFORM_GAMEPAD_BUTTON_RIGHT_SHOULDER }, // BTN_TR
	{ 0x13a, VOE_PLATFORM_GAMEPAD_BUTTON_BACK },	       // BTN_SELECT
	{ 0x13b, VOE_PLATFORM_GAMEPAD_BUTTON_START },	       // BTN_START
	{ 0x13d, VOE_PLATFORM_GAMEPAD_BUTTON_LEFT_STICK },     // BTN_THUMBL
	{ 0x13e, VOE_PLATFORM_GAMEPAD_BUTTON_RIGHT_STICK },    // BTN_THUMBR
	{ 0x220, VOE_PLATFORM_GAMEPAD_BUTTON_PAD_UP },	       // BTN_DPAD_UP
	{ 0x221, VOE_PLATFORM_GAMEPAD_BUTTON_PAD_DOWN },       // BTN_DPAD_DOWN
	{ 0x222, VOE_PLATFORM_GAMEPAD_BUTTON_PAD_LEFT },       // BTN_DPAD_LEFT
	{ 0x223, VOE_PLATFORM_GAMEPAD_BUTTON_PAD_RIGHT },      // BTN_DPAD_RIGHT
};

#define EVDEV_BTN_TL2 0x138
#define EVDEV_BTN_TR2 0x139

#define EVDEV_ABS_X 0x00
#define EVDEV_ABS_Y 0x01
#define EVDEV_ABS_Z 0x02
#define EVDEV_ABS_RX 0x03
#define EVDEV_ABS_RY 0x04
#define EVDEV_ABS_RZ 0x05
#define EVDEV_ABS_HAT0X 0x10
#define EVDEV_ABS_HAT0Y 0x11

void voe_platform_gamepad_evdev_key(voe_platform_gamepad *pad,
				    const struct voe_platform_gamepad_evdev *device,
				    uint16_t code, int32_t value)
{
	bool down = value != 0;

	VOE_BASE_DEBUG_ASSERT(pad != NULL && device != NULL,
			      "an evdev key for no pad or no device");

	for (size_t i = 0; i < sizeof(evdev_buttons) / sizeof(evdev_buttons[0]); i++) {
		if (evdev_buttons[i].code == code) {
			pad->buttons[evdev_buttons[i].button] = down;
			return;
		}
	}
	if (device->has_trigger_axes)
		return;
	if (code == EVDEV_BTN_TL2)
		pad->left_trigger = down ? 1.0f : 0.0f;
	else if (code == EVDEV_BTN_TR2)
		pad->right_trigger = down ? 1.0f : 0.0f;
}

void voe_platform_gamepad_evdev_abs(voe_platform_gamepad *pad,
				    const struct voe_platform_gamepad_evdev *device,
				    uint16_t code, int32_t value)
{
	const struct voe_platform_gamepad_range *range;

	VOE_BASE_DEBUG_ASSERT(pad != NULL && device != NULL,
			      "an evdev axis for no pad or no device");

	if (code >= sizeof(device->abs) / sizeof(device->abs[0]))
		return;
	range = &device->abs[code];
	switch (code) {
	case EVDEV_ABS_X:
		pad->left_x = voe_platform_gamepad_axis(value, range->min, range->max);
		break;
	case EVDEV_ABS_Y:
		pad->left_y = -voe_platform_gamepad_axis(value, range->min, range->max);
		break;
	case EVDEV_ABS_RX:
		pad->right_x = voe_platform_gamepad_axis(value, range->min, range->max);
		break;
	case EVDEV_ABS_RY:
		pad->right_y = -voe_platform_gamepad_axis(value, range->min, range->max);
		break;
	case EVDEV_ABS_Z:
		pad->left_trigger = voe_platform_gamepad_trigger(value, range->min, range->max);
		break;
	case EVDEV_ABS_RZ:
		pad->right_trigger = voe_platform_gamepad_trigger(value, range->min, range->max);
		break;
	case EVDEV_ABS_HAT0X:
		pad->buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_LEFT] = value < 0;
		pad->buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_RIGHT] = value > 0;
		break;
	case EVDEV_ABS_HAT0Y:
		pad->buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_UP] = value < 0;
		pad->buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_DOWN] = value > 0;
		break;
	default:
		break;
	}
}
