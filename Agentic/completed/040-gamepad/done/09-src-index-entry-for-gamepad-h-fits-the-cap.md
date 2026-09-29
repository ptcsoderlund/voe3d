# 09 — The src index entry for gamepad.h fits the cap
folder: platform/src
after: none
decisions: 0168, 0292

## Change
`platform/src/src.md`: the `gamepad.h` entry is 351 characters; the cap is
300. Cut it to one sentence under 300: the four pad slots, attach and detach,
and evdev, XInput and HID input mapped onto a pad, OS-free. The rest (range
onto −1..1 or 0..1, why OS-free and built on both platforms, why digital
triggers yield to axes, why HID is PlayStation order) belongs in the header
comment of `platform/src/gamepad.h`; open that header comment only and add
any of those points it does not already make. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder platform/src` prints
no finding for `platform/src/src.md`.
