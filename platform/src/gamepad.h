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
#pragma once

#include <platform/input.h>

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
