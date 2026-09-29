// The gamepad slots and the normalising every backend shares (ADR-0292).
// Internal to this folder, and built and tested on both platforms because it
// includes no OS header, as keymap.h does.
//
// WHY OS-FREE (ADR-0292 point 6). Which slot a pad takes, what id it gets and
// how a device's raw range becomes −1..1 are the same answer on Linux and
// Windows, and they are where a bug would make the two platforms disagree. So
// they are here once, tested without a device, and the _wayland and _win32 files
// only move bytes: they call attach when a pad arrives, detach when it goes, and
// write the slot's fields through the two conversions below.
//
// A CALLER INVERTS A DOWN-POSITIVE Y. evdev and HID report a stick pushed down
// as larger; the public header promises up positive,
// so a backend negates what voe_platform_gamepad_axis gives back for such an
// axis. The conversions themselves know nothing of direction.
//
// EVDEV CODES ARE THE KERNEL'S STANDARD GAMEPAD MAPPING (ADR-0292 point 4).
// Every common pad's driver maps onto BTN_SOUTH.., ABS_X.. and ABS_HAT0X, so the
// evdev functions below read those codes and nothing device-specific. They are
// written as numbers with the kernel's names beside them, as keymap.c does, so
// no linux/input.h is needed and this file still builds on Windows.
//
// DIGITAL TRIGGERS YIELD TO AXES. A pad with ABS_Z/ABS_RZ reports its triggers
// there in full range and often also as BTN_TL2/BTN_TR2 once past a threshold;
// taking both would snap an analogue trigger to 0 or 1. So the buttons move a
// trigger only on a device with no trigger axes.
//
// HID IS READ IN PLAYSTATION ORDER (ADR-0292 point 5). XInput covers Xbox pads;
// what reaches Windows as HID is mostly PlayStation pads, and HID names no
// place for a button, only a number. So the numbers are read as a DualShock or
// DualSense lays them out. A generic pad reads as that order says: its button
// 1 is west and its Z/Rz the right stick, whatever is printed on it.
#pragma once

#include <platform/input.h>

#include <stdbool.h>
#include <stdint.h>

// Every slot a window has, and the id the last attached pad was given. Lives in
// struct voe_platform_input; zeroed, it is four empty slots.
struct voe_platform_gamepads {
	voe_platform_gamepad slots[VOE_PLATFORM_GAMEPAD_SLOTS];
	uint32_t next_id;
};

// Takes the lowest free slot, marks it connected and at rest with an id one
// higher than any given before, and returns its index; −1 when all four are
// taken, and then nothing changes.
int voe_platform_gamepad_attach(struct voe_platform_gamepads *pads);

// Puts slot back to all zero: not connected, id 0, at rest.
void voe_platform_gamepad_detach(struct voe_platform_gamepads *pads, int slot);

// value in min..max onto −1..1 about the middle of the range, clamped past
// either end. A range of no width, or one backwards, is 0.
float voe_platform_gamepad_axis(int32_t value, int32_t min, int32_t max);

// value in min..max onto 0..1, clamped. A range of no width, or one backwards,
// is 0.
float voe_platform_gamepad_trigger(int32_t value, int32_t min, int32_t max);

// One axis's range as the device reported it.
struct voe_platform_gamepad_range {
	int32_t min;
	int32_t max;
};

// An evdev device's axis ranges by ABS_* code, filled by the backend from
// EVIOCGABS, and whether it has ABS_Z/ABS_RZ.
struct voe_platform_gamepad_evdev {
	struct voe_platform_gamepad_range abs[64];
	bool has_trigger_axes;
};

// An EV_KEY event onto pad: face buttons, shoulders, back, start, sticks and
// BTN_DPAD_*; BTN_TL2/TR2 set a trigger to 0 or 1 only without trigger axes.
// value 0 is up, anything else down. A code not listed is ignored.
void voe_platform_gamepad_evdev_key(voe_platform_gamepad *pad,
				    const struct voe_platform_gamepad_evdev *device,
				    uint16_t code, int32_t value);

// An EV_ABS event onto pad: ABS_X/Y left and ABS_RX/RY right stick with Y
// inverted, ABS_Z/RZ the triggers, ABS_HAT0X/Y the pad's four buttons (−1 left
// or up). Other codes are ignored.
void voe_platform_gamepad_evdev_abs(voe_platform_gamepad *pad,
				    const struct voe_platform_gamepad_evdev *device,
				    uint16_t code, int32_t value);

// A whole XINPUT_GAMEPAD onto pad: A south, B east, X west, Y north, shoulders,
// BACK, START, thumbs and DPAD; triggers over 0..255; sticks over
// −32768..32767, whose Y is already up positive and so not inverted.
void voe_platform_gamepad_xinput(voe_platform_gamepad *pad, uint16_t buttons,
				 uint8_t left_trigger, uint8_t right_trigger, int16_t left_x,
				 int16_t left_y, int16_t right_x, int16_t right_y);

// One HID generic desktop value onto pad, over the range its report declares:
// X/Y left and Z/Rz right stick with Y and Rz inverted, Rx/Ry the left and right
// triggers, the hat switch (value − range.min of 0..7 clockwise from up) the
// pad's four buttons, any other hat value releasing them. Other usages are
// ignored.
void voe_platform_gamepad_hid_value(voe_platform_gamepad *pad, uint16_t usage, int32_t value,
				    struct voe_platform_gamepad_range range);

// The HID button usages down now, count of them: 1 west, 2 south, 3 east,
// 4 north, 5 and 6 shoulders, 9 back, 10 start, 11 and 12 sticks. Every one of
// those not listed goes up; the pad's four buttons are the hat's, untouched.
void voe_platform_gamepad_hid_buttons(voe_platform_gamepad *pad, const uint16_t *usages,
				      uint32_t count);
