// What src/gamepad.h promises, checked with no device: four attaches fill
// slots 0 to 3 and a fifth is refused; a slot freed and taken again gets a
// higher id than any before and a freed slot is all zero; an axis and a trigger
// map both ends, the middle and past both ends, and a range of no width is 0.
// An Xbox-like evdev device (sticks −32768..32767, triggers 0..1023) moves a
// pad: stick up reads 1, centre 0, full ABS_RZ 1, BTN_SOUTH goes down and up,
// the hat presses and releases left, BTN_TR2 counts only without trigger axes,
// and an unknown code changes nothing. Codes are the kernel's, as numbers.
// XInput with A and DPAD_LEFT, a full left trigger and left stick up reads
// south, left, 1 and 1, and all zero reads at rest. HID: X at 255 of 0..255 is
// left_x 1, Rz at 0 is right_y 1, hat 2 presses right only and 8 releases, and
// buttons {2, 10} press south and start until {} lets them go.
//
// Needs no window, no display and no pad.
#include "../src/gamepad.h"

#include <testing/test.h>

#include <string.h>

static void check_slots(void)
{
	struct voe_platform_gamepads pads = { 0 };
	voe_platform_gamepad zero = { 0 };
	uint32_t highest = 0;

	for (int slot = 0; slot < VOE_PLATFORM_GAMEPAD_SLOTS; slot++) {
		VOE_TEST_CHECK_INT(voe_platform_gamepad_attach(&pads), slot);
		VOE_TEST_CHECK(pads.slots[slot].connected);
		VOE_TEST_CHECK(pads.slots[slot].id > highest);
		highest = pads.slots[slot].id;
	}
	VOE_TEST_CHECK_INT(voe_platform_gamepad_attach(&pads), -1);

	pads.slots[1].left_x = 0.5f;
	pads.slots[1].buttons[VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH] = true;
	voe_platform_gamepad_detach(&pads, 1);
	VOE_TEST_CHECK(memcmp(&pads.slots[1], &zero, sizeof(zero)) == 0);

	VOE_TEST_CHECK_INT(voe_platform_gamepad_attach(&pads), 1);
	VOE_TEST_CHECK(pads.slots[1].connected);
	VOE_TEST_CHECK(pads.slots[1].id > highest);
	VOE_TEST_CHECK(pads.slots[1].left_x == 0.0f);
	VOE_TEST_CHECK(!pads.slots[1].buttons[VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH]);
}

static void check_axis(void)
{
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(-32768, -32768, 32767),
			     -1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(32767, -32768, 32767),
			     1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(128, 0, 256), 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(64, 0, 256), -0.5f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(-10, 0, 255), -1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(300, 0, 255), 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(5, 7, 7), 0.0f, 0.0f);
}

static void check_trigger(void)
{
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(0, 0, 255), 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(255, 0, 255), 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(128, 0, 256), 0.5f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(-5, 0, 255), 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(999, 0, 255), 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(3, 3, 3), 0.0f, 0.0f);
}

static struct voe_platform_gamepad_evdev xbox_like(void)
{
	struct voe_platform_gamepad_evdev device = { 0 };
	const struct voe_platform_gamepad_range stick = { -32768, 32767 };
	const struct voe_platform_gamepad_range trigger = { 0, 1023 };

	device.abs[0x00] = stick;   // ABS_X
	device.abs[0x01] = stick;   // ABS_Y
	device.abs[0x03] = stick;   // ABS_RX
	device.abs[0x04] = stick;   // ABS_RY
	device.abs[0x02] = trigger; // ABS_Z
	device.abs[0x05] = trigger; // ABS_RZ
	device.abs[0x10] = (struct voe_platform_gamepad_range){ -1, 1 }; // ABS_HAT0X
	device.abs[0x11] = (struct voe_platform_gamepad_range){ -1, 1 }; // ABS_HAT0Y
	device.has_trigger_axes = true;
	return device;
}

static void check_evdev(void)
{
	struct voe_platform_gamepad_evdev device = xbox_like();
	voe_platform_gamepad pad = { 0 };
	voe_platform_gamepad before;

	voe_platform_gamepad_evdev_abs(&pad, &device, 0x01, -32768); // ABS_Y
	VOE_TEST_CHECK_FLOAT(pad.left_y, 1.0f, 1e-6f);
	voe_platform_gamepad_evdev_abs(&pad, &device, 0x03, 0); // ABS_RX
	VOE_TEST_CHECK_FLOAT(pad.right_x, 0.0f, 1e-4f);
	voe_platform_gamepad_evdev_abs(&pad, &device, 0x05, 1023); // ABS_RZ
	VOE_TEST_CHECK_FLOAT(pad.right_trigger, 1.0f, 1e-6f);

	voe_platform_gamepad_evdev_key(&pad, &device, 0x130, 1); // BTN_SOUTH
	VOE_TEST_CHECK(pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH]);
	voe_platform_gamepad_evdev_key(&pad, &device, 0x130, 0);
	VOE_TEST_CHECK(!pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH]);

	voe_platform_gamepad_evdev_abs(&pad, &device, 0x10, -1); // ABS_HAT0X
	VOE_TEST_CHECK(pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_LEFT]);
	VOE_TEST_CHECK(!pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_RIGHT]);
	voe_platform_gamepad_evdev_abs(&pad, &device, 0x10, 0);
	VOE_TEST_CHECK(!pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_LEFT]);

	voe_platform_gamepad_evdev_key(&pad, &device, 0x139, 1); // BTN_TR2
	VOE_TEST_CHECK_FLOAT(pad.right_trigger, 1.0f, 1e-6f);
	voe_platform_gamepad_evdev_abs(&pad, &device, 0x05, 0);
	voe_platform_gamepad_evdev_key(&pad, &device, 0x139, 1);
	VOE_TEST_CHECK_FLOAT(pad.right_trigger, 0.0f, 0.0f);
	device.has_trigger_axes = false;
	voe_platform_gamepad_evdev_key(&pad, &device, 0x139, 1);
	VOE_TEST_CHECK_FLOAT(pad.right_trigger, 1.0f, 0.0f);
	voe_platform_gamepad_evdev_key(&pad, &device, 0x139, 0);
	VOE_TEST_CHECK_FLOAT(pad.right_trigger, 0.0f, 0.0f);

	before = pad;
	voe_platform_gamepad_evdev_key(&pad, &device, 0x2c0, 1);
	voe_platform_gamepad_evdev_abs(&pad, &device, 0x28, 5);
	voe_platform_gamepad_evdev_abs(&pad, &device, 0x3f, 5);
	voe_platform_gamepad_evdev_abs(&pad, &device, 0x100, 5);
	VOE_TEST_CHECK(memcmp(&pad, &before, sizeof(pad)) == 0);
}

static void check_xinput(void)
{
	voe_platform_gamepad pad = { 0 };

	// XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_DPAD_LEFT
	voe_platform_gamepad_xinput(&pad, 0x1000 | 0x0004, 255, 0, 0, 32767, 0, 0);
	VOE_TEST_CHECK(pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH]);
	VOE_TEST_CHECK(pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_LEFT]);
	VOE_TEST_CHECK(!pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_EAST]);
	VOE_TEST_CHECK_FLOAT(pad.left_trigger, 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(pad.left_y, 1.0f, 1e-6f);

	voe_platform_gamepad_xinput(&pad, 0, 0, 0, 0, 0, 0, 0);
	for (int button = 0; button < VOE_PLATFORM_GAMEPAD_BUTTON_COUNT; button++)
		VOE_TEST_CHECK(!pad.buttons[button]);
	VOE_TEST_CHECK_FLOAT(pad.left_trigger, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(pad.right_trigger, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(pad.left_x, 0.0f, 1.0f / 32767.0f);
	VOE_TEST_CHECK_FLOAT(pad.left_y, 0.0f, 1.0f / 32767.0f);
	VOE_TEST_CHECK_FLOAT(pad.right_x, 0.0f, 1.0f / 32767.0f);
	VOE_TEST_CHECK_FLOAT(pad.right_y, 0.0f, 1.0f / 32767.0f);
}

static void check_hid(void)
{
	const struct voe_platform_gamepad_range byte = { 0, 255 };
	const struct voe_platform_gamepad_range hat = { 0, 7 };
	const uint16_t south_start[] = { 2, 10 };
	voe_platform_gamepad pad = { 0 };

	voe_platform_gamepad_hid_value(&pad, 0x30, 255, byte); // X
	VOE_TEST_CHECK_FLOAT(pad.left_x, 1.0f, 1e-6f);
	voe_platform_gamepad_hid_value(&pad, 0x35, 0, byte); // Rz
	VOE_TEST_CHECK_FLOAT(pad.right_y, 1.0f, 1e-6f);

	voe_platform_gamepad_hid_value(&pad, 0x39, 2, hat); // hat switch, right
	VOE_TEST_CHECK(pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_RIGHT]);
	VOE_TEST_CHECK(!pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_UP]);
	VOE_TEST_CHECK(!pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_DOWN]);
	VOE_TEST_CHECK(!pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_LEFT]);
	voe_platform_gamepad_hid_value(&pad, 0x39, 8, hat); // hat switch, released
	VOE_TEST_CHECK(!pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_RIGHT]);

	voe_platform_gamepad_hid_buttons(&pad, south_start, 2);
	VOE_TEST_CHECK(pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH]);
	VOE_TEST_CHECK(pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_START]);
	VOE_TEST_CHECK(!pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_WEST]);
	voe_platform_gamepad_hid_buttons(&pad, NULL, 0);
	VOE_TEST_CHECK(!pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH]);
	VOE_TEST_CHECK(!pad.buttons[VOE_PLATFORM_GAMEPAD_BUTTON_START]);
}

int main(void)
{
	check_slots();
	check_axis();
	check_trigger();
	check_evdev();
	check_xinput();
	check_hid();
	return voe_test_result();
}
