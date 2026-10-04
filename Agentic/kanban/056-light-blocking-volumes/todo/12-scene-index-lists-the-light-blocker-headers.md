# 12 — The scene header index lists the light blocker headers
folder: scene/include/scene
after: none
decisions: 0168, 0347

## Change
`scene/include/scene/scene.md` lacks entries for two headers that exist.
Add, after the `point_light_system.h` entry and in the same shape as the
others (one phrase each, under 300 characters):
- `light_blocker_component.h` — the box that keeps outside light out and
  inside light in: its size along its transform's axes.
- `light_blocker_system.h` — the replace intent, the direct call that creates
  one, and the run that drains it.
Read the top comment of each header to word them. Change no header.

## Done when
`grep -c 'light_blocker_component.h\|light_blocker_system.h' scene/include/scene/scene.md`
prints 2 or more, and `checks.sh --folder scene/include/scene` reports no finding.
