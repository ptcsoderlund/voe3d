# 13 — The render index entry for device.h fits its cap
folder: render/include/render
after: none
decisions: 0168

## Change
Documentation only; no code, no signature changes.

`render/include/render/render.md`: the entry for `device.h` is 441
characters, the cap is 300. Cut it to one sentence under 300 characters
naming what the header is: the whole public surface of the device (window,
uploads for ids, frames of passes onto the window or a target).

`render/include/render/device.h`, header comment only: make sure the points
the entry drops are made there — passes after the sun's shadow cascades and
bounce pass, the bounce grid's sizes, the update that refreshes a target's
grid from the bounce map, a pass copying depth, and water, waves and sky in a
shading record. Add only what is missing; touch nothing below the header.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `render/include/render/render.md`.
