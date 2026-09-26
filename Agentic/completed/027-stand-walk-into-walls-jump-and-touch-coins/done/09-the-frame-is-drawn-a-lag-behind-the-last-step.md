# 09 — The frame is drawn a lag behind the last step
folder: 3d
decisions: 0168, 0254, 0065

## Change
0254 point 4: the draw interpolates between the last two steps (ADR-0065 point 4) through
scene's `voe_scene_transform_between` (card 04). `dev`, `game` and `editor` mend the call.

- `3d/include/3d/draw_system.h`, `3d/src/draw_system.c` —
  `voe_3d_draw_system_frame(world, size, float lag)`; `voe_3d_frame` gains `float lag`. The camera
  pose `_frame` builds the view and `eye` from, and every mesh and panel transform `_run` draws,
  come from `voe_scene_transform_between(world, entity, frame.lag)`. The outline, gizmo and marker
  keep the current transform (they are the editor's, which passes 0). Header points: what lag is
  (0254: a fraction of a step back, 0 now); a program that does not step passes 0.
- `3d/src/draw_group.c` — only if `object_of` or the sort takes the transform there.
- `3d/tests/draw_system.c` (or `far.c`) — a headless world with the previous table registered,
  a camera and a cube remembered, then moved 1 m along X: `_frame` at lag 0.5 has `eye` halfway;
  at lag 0 it is where the camera is now. Every other test's `_frame` call passes 0.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^3d/draw_system$'` passes.
