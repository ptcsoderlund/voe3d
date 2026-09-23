# 02 — `3d` sees through a render view built from a pose and a lens
folder: 3d
decisions: 0168, 0222, 0223

## Change
Card 01 made the camera a lens and put its pose in the transform. `3d` follows.

- `3d/include/3d/projection.h` / `3d/src/projection.c` — `voe_3d_projection(voe_scene_camera lens, float
  aspect)` keeps its signature; the header says the camera is the lens. New:
  `[[nodiscard]] bool voe_3d_view(voe_scene_transform pose, voe_scene_camera lens, float aspect,
  voe_render_view *out)`: view from `voe_scene_camera_view(pose, …)`, projection from the lens, eye the pose's
  position, `reserved` 0; false with `out` untouched when the pose sees nothing. Header points: this is the one
  place a pose and a lens become what `render`, the outline, the gizmo and the pick are handed (0223); the world's
  camera and an editor's orbit both come through it.
- `3d/include/3d/pick.h` / `3d/src/pick.c` — `voe_3d_pick_ray(voe_render_view view, voe_platform_size size,
  voe_math_float2 point)`: inverts `view.projection · view.view`; the header's paragraph on which two matrices it
  builds becomes that it is handed them.
- `3d/include/3d/draw_system.h` / `3d/src/draw_system.c` — `voe_3d_frame` gains `bool blind`: true when the
  camera sees nothing, zero meaning seen (0223). `voe_3d_draw_system_frame` reads camera row 0, its entity's
  transform (assert it has one: the camera needs one) and builds `frame.view` with `voe_3d_view`; on false it
  sets `blind` and leaves `view` zeroed. `voe_3d_draw_system_run` returns at once when `frame.blind`. Header:
  the frame paragraphs say the camera's pose is its transform, and what `blind` means.
- `3d/tests/projection.c`, `pick.c`, `draw_system.c`, `gizmo.c`, `import.c`, `outline.c`, `panel.c` — every
  world registers the transform before the camera and gives the camera entity a transform; every camera value
  built by hand becomes a pose and a lens through `voe_3d_view`. New checks in `projection.c`: `voe_3d_view` of
  a pose at (0, 0, 5) gives eye (0, 0, 5) and matches `voe_scene_camera_view`; a zero-scale pose returns false.
  New check in `draw_system.c`: a world whose camera's transform has a zero scale frames `blind`.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — entries for projection, pick and draw_system follow.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `3d` exits 0, and
`grep -rnE "\.yaw|\.pitch|camera\.eye|camera_forward" 3d` prints nothing.
