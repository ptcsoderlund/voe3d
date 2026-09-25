# 07 — Split the draw system by what it draws
folder: 3d
decisions: 0168

## Change
`3d/src/draw_system.c` is 778 lines and cards 08, 09 and 14 all add to it. Split it by
function first, no behaviour change.

- `3d/src/draw_group.h` + `draw_group.c` (new) — the drawables held back and drawn in order:
  `struct group`, `struct deferred`, `group_new`, `hold`, `draw_group`, `object_of`,
  `view_depth`, `shape_type`, `is_the_same_entity`, as internal calls with the `voe_3d_` prefix
  their new linkage needs. Header points: the four groups and why three are held back
  (from `draw_system.h`'s TWO PASSES / TWO LAYERS paragraphs, referred to, not copied).
- `3d/src/draw_marks.h` + `draw_marks.c` (new) — what a pass draws on top of the world for the
  editor: the camera marker (`draw_camera_marker`), the outline's quads and draw (the block around
  `voe_3d_outline_quads` in `_run`, as one call) and the gizmo's (`draw_gizmo_mesh` and the block
  that builds and draws the two meshes, as one call), each taking the frame, device and arena.
  Header point: why these come after the world and behind their own depth clears (ADR-0203,
  ADR-0205, 0223), referring to `draw_system.h`.
- `3d/src/draw_system.c` — keeps `voe_3d_draw_system_light`, `_frame`, the panel helpers and
  `_run`, which calls the above.
- `3d/src/src.md` — entries for the four new files; `draw_system.c`'s narrowed.

No public header changes.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^3d/'` passes with no test file edited.
3. `wc -l 3d/src/draw_system.c` is under 500.

## Blocked
The split is done (draw_group.{h,c}, draw_marks.{h,c}, draw_system.c at 393 lines, src.md) and
the folder's structural checks pass, but `3d` does not build: cards 03/06 changed
`voe_scene_transform_matrix` to take an origin and made a transform's position a
`voe_math_double3`, so `projection.c`, `import.c`, `camera_marker.c`, `outline.c`, the tests and
the moved code in `draw_group.c`/`draw_marks.c`/`draw_system.c` fail exactly as HEAD's
`draw_system.c` did. Card 08 (3d draws, picks and moves about the eye) fixes those callers; once it
lands, Done when 1 and 2 should pass with no further change here.
