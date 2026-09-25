# 08 — 3d draws, picks and moves things about the eye
folder: 3d
decisions: 0168, 0250, 0177

## Change
0250 in `3d`: a transform's position is double (card 03) and the GPU sees only floats about the
eye. `render` is untouched: a `voe_render_view` is now eye-relative, its `eye` zero. `dev`,
`game` and `editor` are mended by their own cards. Signatures:

- `include/3d/projection.h`, `src/projection.c` — `voe_3d_view` keeps its signature; the view it
  builds is about the pose's own position (card 03's camera view), `out->eye` zero. Header point:
  every render view is eye-relative; the eye's world position travels beside it as a
  `voe_math_double3`.
- `include/3d/draw_system.h`, `src/draw_system.c`, `src/draw_group.c`, `src/draw_marks.c` (card 07)
  — `voe_3d_frame` gains `voe_math_double3 eye`, set by `_frame` from the camera's position; every
  object's matrix is `voe_scene_transform_matrix(t, frame.eye)`, and the sort, panel, outline,
  gizmo and marker all use the same eye.
- `include/3d/pick.h`, `src/pick.c` — `voe_3d_ray.origin` is `voe_math_double3`;
  `voe_3d_pick_ray(view, voe_math_double3 eye, size, point)`; `voe_3d_pick` tests each entity in
  float about the ray's origin.
- `include/3d/gizmo.h`, `src/gizmo.c` — `voe_3d_gizmo.origin` and `.eye` are double3;
  `voe_3d_gizmo_at(voe_math_double3 origin, view, voe_math_double3 eye, size, pixels)`;
  `voe_3d_gizmo_grab`'s `out` is double3; `_quads` builds its vertices about `gizmo.eye`.
- `include/3d/outline.h`, `src/outline.c` — `voe_3d_outline_quads(world, outlined, view,
  voe_math_double3 eye, arena, out)`, quads about the eye.
- `include/3d/camera_marker.h`, `src/camera_marker.c` — `_quads(pose, lens, view,
  voe_math_double3 eye, size, pixels, arena, out)` about the eye; `_hit` in float about the
  ray's origin.
- `src/import.c` — a node's translation widened into the double position.
- every `3d/tests/*.c` that places, picks, grabs or outlines follows; new `3d/tests/far.c`: with
  everything moved by (100000, 0, 100000), a pick through the picture's centre hits the cube it
  hit at the origin at the same distance ±1e-4; a gizmo grab 1 mm along X reads back 1 mm ±1e-5;
  the frame's `eye` is the camera's double position and the cube's drawn matrix translation equals
  its offset from the eye. List it on `3d/tests/tests.md`.
- card 07 split `draw_system.c` into `draw_group.c` and `draw_marks.c` but could not build
  (this card's callers were broken); its proof lands here. Keep the split: mend the moved code
  in place, move nothing back.
- `3d/3d.md`, `3d/src/src.md` — entries that say "world space" for a quad or a ray say "about
  the eye" where that is now true.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^3d/'` passes, `3d/far` among them.
3. `wc -l 3d/src/draw_system.c` is under 500.
