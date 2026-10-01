# 13 — The render headers' index entry fits the cap
folder: render/include/render
after: none
decisions: 0168

## Change
In `render/include/render/render.md`, the entry `device.h` is 301 characters;
the cap is 300. Shorten it to one sentence under the cap, keeping the points:
whole public surface, device on a window, uploads for ids, a frame of passes
onto window or target with depth copy, water as a shading record. Change only
the `.md`; `device.h` already carries the fuller account.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render/include/render`
prints `FINDINGS: 0`.
