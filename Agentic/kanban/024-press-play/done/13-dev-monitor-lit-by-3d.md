# 13 — dev's monitor is lit by 3d's answer
folder: dev
decisions: 0168, 0238

## Change
`voe_dev_monitor_frame` asserts on its own that the world has one light and copies the row, and its
header says it asserts "for the same reason" `voe_3d_draw_system_frame` does, which card 11 makes
untrue. Use the owner instead.

- `dev/src/monitor.c` — in `voe_dev_monitor_frame`, the light assert and the row read go; the
  frame's light is `voe_3d_draw_system_light(world)`. Drop the `scene/light_component.h` include if
  nothing else uses it.
- `dev/src/monitor.h` — the paragraph on `voe_dev_monitor_frame` says the light is `3d`'s answer,
  unshaded when the world has none (0238); the "already requires" sentence goes. The header's
  opening lines, if they name the light requirement, follow.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder dev` prints `FINDINGS: 0`.
2. `grep -c voe_scene_light_count dev/src/monitor.c` prints `0`.
