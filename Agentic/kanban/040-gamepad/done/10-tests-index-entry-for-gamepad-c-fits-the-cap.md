# 10 — The tests index entry for gamepad.c fits the cap
folder: platform/tests
after: none
decisions: 0168, 0292

## Change
`platform/tests/tests.md`: the `gamepad.c` entry is 383 characters; the cap
is 300. Cut it to one sentence under 300: slot attach and detach, range
mapping, and evdev, XInput and HID input onto a pad. The case-by-case list
(fifth attach refused, freed slot zero with a higher id, axis ends and no
width, the Xbox-like evdev device's codes, XInput pressed and at rest, HID
in PlayStation order) belongs in the header comment of
`platform/tests/gamepad.c`; open that header comment only and add any of
those points it does not already make. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder platform/tests`
prints no finding for `platform/tests/tests.md`.
