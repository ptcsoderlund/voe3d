// The gamepad slots and range conversions of src/gamepad.h, once, for both
// platforms. No OS header and no #ifdef: a backend calls these with what its
// device reported, and nothing here knows which device that was.
//
// The conversions work in double so that a 32-bit range's width cannot
// overflow and its middle is exact. The evdev translation is a table of the
// kernel's button codes and a switch over its axis codes; the numbers carry
// the kernel's names beside them because linux/input.h is not included. The
// XInput bits and HID usages are numbers the same way, with the SDK's and the
// HID usage tables' names beside them, so no Windows header is needed either.
//
// The HID button scan is count × twelve comparisons; a report holds a few
// dozen usages at most, so a lookup table would buy nothing.
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

// XINPUT_GAMEPAD wButtons bits, the SDK's names beside them.
static const struct {
	uint16_t bit;
	voe_platform_gamepad_button button;
} xinput_buttons[] = {
	{ 0x0001, VOE_PLATFORM_GAMEPAD_BUTTON_PAD_UP },	       // XINPUT_GAMEPAD_DPAD_UP
	{ 0x0002, VOE_PLATFORM_GAMEPAD_BUTTON_PAD_DOWN },      // XINPUT_GAMEPAD_DPAD_DOWN
	{ 0x0004, VOE_PLATFORM_GAMEPAD_BUTTON_PAD_LEFT },      // XINPUT_GAMEPAD_DPAD_LEFT
	{ 0x0008, VOE_PLATFORM_GAMEPAD_BUTTON_PAD_RIGHT },     // XINPUT_GAMEPAD_DPAD_RIGHT
	{ 0x0010, VOE_PLATFORM_GAMEPAD_BUTTON_START },	       // XINPUT_GAMEPAD_START
	{ 0x0020, VOE_PLATFORM_GAMEPAD_BUTTON_BACK },	       // XINPUT_GAMEPAD_BACK
	{ 0x0040, VOE_PLATFORM_GAMEPAD_BUTTON_LEFT_STICK },    // XINPUT_GAMEPAD_LEFT_THUMB
	{ 0x0080, VOE_PLATFORM_GAMEPAD_BUTTON_RIGHT_STICK },   // XINPUT_GAMEPAD_RIGHT_THUMB
	{ 0x0100, VOE_PLATFORM_GAMEPAD_BUTTON_LEFT_SHOULDER }, // XINPUT_GAMEPAD_LEFT_SHOULDER
	{ 0x0200, VOE_PLATFORM_GAMEPAD_BUTTON_RIGHT_SHOULDER }, // XINPUT_GAMEPAD_RIGHT_SHOULDER
	{ 0x1000, VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH },	       // XINPUT_GAMEPAD_A
	{ 0x2000, VOE_PLATFORM_GAMEPAD_BUTTON_EAST },	       // XINPUT_GAMEPAD_B
	{ 0x4000, VOE_PLATFORM_GAMEPAD_BUTTON_WEST },	       // XINPUT_GAMEPAD_X
	{ 0x8000, VOE_PLATFORM_GAMEPAD_BUTTON_NORTH },	       // XINPUT_GAMEPAD_Y
};

void voe_platform_gamepad_xinput(voe_platform_gamepad *pad, uint16_t buttons,
				 uint8_t left_trigger, uint8_t right_trigger, int16_t left_x,
				 int16_t left_y, int16_t right_x, int16_t right_y)
{
	VOE_BASE_DEBUG_ASSERT(pad != NULL, "an XInput state for no pad");

	for (size_t i = 0; i < sizeof(xinput_buttons) / sizeof(xinput_buttons[0]); i++)
		pad->buttons[xinput_buttons[i].button] = (buttons & xinput_buttons[i].bit) != 0;
	pad->left_trigger = voe_platform_gamepad_trigger(left_trigger, 0, 255);
	pad->right_trigger = voe_platform_gamepad_trigger(right_trigger, 0, 255);
	pad->left_x = voe_platform_gamepad_axis(left_x, -32768, 32767);
	pad->left_y = voe_platform_gamepad_axis(left_y, -32768, 32767);
	pad->right_x = voe_platform_gamepad_axis(right_x, -32768, 32767);
	pad->right_y = voe_platform_gamepad_axis(right_y, -32768, 32767);
}

// HID generic desktop usages (page 0x01), the table's names beside them.
#define HID_USAGE_X 0x30
#define HID_USAGE_Y 0x31
#define HID_USAGE_Z 0x32
#define HID_USAGE_RX 0x33
#define HID_USAGE_RY 0x34
#define HID_USAGE_RZ 0x35
#define HID_USAGE_HAT_SWITCH 0x39

// The pad buttons each hat position holds down, 0 up and clockwise by eighths.
static const uint8_t hat_directions[8] = {
	1u << 0,	       // up
	1u << 0 | 1u << 3, // up right
	1u << 3,	       // right
	1u << 1 | 1u << 3, // down right
	1u << 1,	       // down
	1u << 1 | 1u << 2, // down left
	1u << 2,	       // left
	1u << 0 | 1u << 2, // up left
};

static void hid_hat(voe_platform_gamepad *pad, int64_t position)
{
	uint8_t held = 0;

	if (position >= 0 && position < 8)
		held = hat_directions[position];
	pad->buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_UP] = (held & 1u << 0) != 0;
	pad->buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_DOWN] = (held & 1u << 1) != 0;
	pad->buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_LEFT] = (held & 1u << 2) != 0;
	pad->buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_RIGHT] = (held & 1u << 3) != 0;
}

void voe_platform_gamepad_hid_value(voe_platform_gamepad *pad, uint16_t usage, int32_t value,
				    struct voe_platform_gamepad_range range)
{
	VOE_BASE_DEBUG_ASSERT(pad != NULL, "a HID value for no pad");

	switch (usage) {
	case HID_USAGE_X:
		pad->left_x = voe_platform_gamepad_axis(value, range.min, range.max);
		break;
	case HID_USAGE_Y:
		pad->left_y = -voe_platform_gamepad_axis(value, range.min, range.max);
		break;
	case HID_USAGE_Z:
		pad->right_x = voe_platform_gamepad_axis(value, range.min, range.max);
		break;
	case HID_USAGE_RZ:
		pad->right_y = -voe_platform_gamepad_axis(value, range.min, range.max);
		break;
	case HID_USAGE_RX:
		pad->left_trigger = voe_platform_gamepad_trigger(value, range.min, range.max);
		break;
	case HID_USAGE_RY:
		pad->right_trigger = voe_platform_gamepad_trigger(value, range.min, range.max);
		break;
	case HID_USAGE_HAT_SWITCH:
		hid_hat(pad, (int64_t)value - (int64_t)range.min);
		break;
	default:
		break;
	}
}

// HID button usages (page 0x09) in PlayStation order.
static const struct {
	uint16_t usage;
	voe_platform_gamepad_button button;
} hid_buttons[] = {
	{ 1, VOE_PLATFORM_GAMEPAD_BUTTON_WEST },	   // square
	{ 2, VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH },	   // cross
	{ 3, VOE_PLATFORM_GAMEPAD_BUTTON_EAST },	   // circle
	{ 4, VOE_PLATFORM_GAMEPAD_BUTTON_NORTH },	   // triangle
	{ 5, VOE_PLATFORM_GAMEPAD_BUTTON_LEFT_SHOULDER },  // L1
	{ 6, VOE_PLATFORM_GAMEPAD_BUTTON_RIGHT_SHOULDER }, // R1
	{ 9, VOE_PLATFORM_GAMEPAD_BUTTON_BACK },	   // share
	{ 10, VOE_PLATFORM_GAMEPAD_BUTTON_START },	   // options
	{ 11, VOE_PLATFORM_GAMEPAD_BUTTON_LEFT_STICK },	   // L3
	{ 12, VOE_PLATFORM_GAMEPAD_BUTTON_RIGHT_STICK },   // R3
};

void voe_platform_gamepad_hid_buttons(voe_platform_gamepad *pad, const uint16_t *usages,
				      uint32_t count)
{
	const size_t known = sizeof(hid_buttons) / sizeof(hid_buttons[0]);

	VOE_BASE_DEBUG_ASSERT(pad != NULL, "HID buttons for no pad");
	VOE_BASE_DEBUG_ASSERT(usages != NULL || count == 0, "HID buttons from nowhere");

	for (size_t i = 0; i < known; i++)
		pad->buttons[hid_buttons[i].button] = false;
	for (uint32_t down = 0; down < count; down++) {
		for (size_t i = 0; i < known; i++) {
			if (hid_buttons[i].usage == usages[down])
				pad->buttons[hid_buttons[i].button] = true;
		}
	}
}
