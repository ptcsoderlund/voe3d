# 0292 — Gamepads are four slots of raw state, read through the window
date: 2026-09-29
by: planner

## Decision
For 040, filling in what the feature and 0268 milestone 7 leave to the planner:

1. **A pad is polled state on the window, like the keyboard.** `platform/include/platform/input.h`
   gains `voe_platform_input_gamepad(window, slot)`, handing back a copy: `connected`, an `id`,
   `left_x`, `left_y`, `right_x`, `right_y` in −1..1 (right and up positive, as +Y is up),
   `left_trigger`, `right_trigger` in 0..1, and `buttons[]` indexed by a position enum: south,
   east, west, north, left and right shoulder, back, start, left and right stick, pad up, down,
   left, right. Buttons are named by place, not by label, as a key is (0239: raw, no actions).
2. **Four slots.** A pad plugged in takes the lowest free slot and a fresh `id`, one higher than
   any before it in the run, so a game tells a new pad from an old one in the same slot. A pad
   pulled out reads `connected` false and all at rest; its slot is free again. Found at window
   open, so a pad plugged in before start works from the first frame; a fifth pad is ignored.
3. **No dead zone and no focus gate in `platform`.** Values are the device's, normalised only.
   A dead zone is the game's (0239). Pads are read whether or not the window has focus; Windows
   registers its raw input with the input sink so both platforms agree.
4. **Linux reads evdev.** `/dev/input/event*` opened non-blocking, a device taken when it has
   `BTN_GAMEPAD` or `BTN_JOYSTICK` and `ABS_X`; `inotify` on `/dev/input` for arrival
   (create and attribute change, since udev sets the permission after the node) and a read
   failing `ENODEV` for removal; state resynced by `EVIOCGABS`/`EVIOCGKEY` at open and after
   `SYN_DROPPED`. Mapping by the kernel's standard codes: `ABS_X/Y` left, `ABS_RX/RY` right,
   `ABS_Z/RZ` triggers (digital `BTN_TL2/TR2` as 0 or 1 where there is no axis), `ABS_HAT0X/Y`
   the pad. No library.
5. **Windows reads XInput for Xbox pads and HID raw input for the rest.** `xinput1_4.dll`
   (else `xinput9_1_0.dll`) and `hid.dll` are loaded at run time, as sound loads its device; no
   link change. An unconnected XInput user is asked again at most once a second. HID gamepads
   and joysticks (generic desktop usages 4 and 5) come as `WM_INPUT` with
   `WM_INPUT_DEVICE_CHANGE` for arrival and removal; a device whose name holds `IG_` is XInput's
   and skipped. HID mapping is PlayStation order: X/Y left, Z/Rz right, Rx/Ry triggers, the hat
   the pad, buttons 1–4 west, south, east, north, 5–6 shoulders, 9 back, 10 start, 11–12
   sticks. A generic pad whose axes differ reads as that order says.
6. **The mapping is OS-free.** Slot attach and detach, axis normalising and the evdev, XInput
   and HID translations are one file, `platform/src/gamepad.c`, built and tested on both
   platforms; the `_wayland` and `_win32` files only move bytes.
7. **The tank game reads one control row.** A runtime-only `tank_control`, one row on the
   player's hull, written by its own system first each step: drive, turn, an aim direction on
   screen, fire, and which device is in use. The pad is the lowest connected slot; a radial dead
   zone of 0.2 on each stick, rescaled to 0..1 past it; fire past 0.5 of the right trigger. The
   pad is in use once touched (a stick past its dead zone, a trigger past 0.5, a button) and
   until the keyboard or mouse is (W/A/S/D, Space, a mouse button, the pointer moving), or no
   pad is connected. Left stick drives as W/S and A/D do; the right stick's direction on screen
   is the turret's aim on the ground, held when the stick is let go.

## Reasoning
Polled state beside the keys keeps one way to read input and needs no new accessor into
either window. Evdev is the kernel's own interface and every common pad has a driver mapping
it to the standard codes; joydev is older and loses the names. XInput alone misses
PlayStation pads, and Windows.Gaming.Input is WinRT, awkward from C; raw HID is what reaches
the rest. A dead zone is policy and differs per game, so it is the game's. Rejected: SDL
(a third-party dependency, rule 5), GameInput (not on every Windows), a pad not owned by a
window (both backends already poll through it).

## Replaces
The words "No gamepad" in `platform/include/platform/input.h`'s header.
