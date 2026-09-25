# 14 — 3d draws a collider's lines and names the collider that fits a shape
folder: 3d
decisions: 0168, 0175, 0253, 0250, 0177

## Change
Feature steps 2–3 need the lines; 0253 point 1 gives `3d` the edge: `physics` added to the `3d`
row in `cmake/voe.cmake` (comment names 0253) and to `3d/CMakeLists.txt`'s DEPENDS.

- `3d/include/3d/collider_marker.h` (new), `3d/src/collider_marker.c` (new) — modelled on
  `camera_marker.h`: `VOE_3D_COLLIDER_MARKER_EDGES`, `_VERTICES`, `_INDICES` (the most any kind
  needs); `voe_3d_collider_marker_quads(voe_physics_shape shape, voe_render_view view,
  voe_math_double3 eye, voe_platform_size size, float pixels, voe_base_arena *arena,
  voe_3d_outline_mesh *out)`, line quads about the eye: a box's 12 edges; a sphere's three great
  circles; a capsule's two end circles, its two upright outlines' four lines and the four half
  arcs of its caps; circles of 24 segments. False when `size` has no area. Header points: the
  shape is `physics`'s world shape, so a scaled entity's lines scale with it; why lines and not a
  solid.
- `3d/include/3d/draw_system.h` — `voe_3d_frame` gains `voe_3d_collider_marked collider` {
  `entity`, `pixels`, `size` }, zeroed for none: drawn when the entity has a collider
  (`voe_physics_shape_of`), after the outline, behind the outline's depth clear, with the
  outline's material and colour (`frame.outlined`), so it shows through what stands in front.
  Capacity note as the marker's: one more transient range and object per pass.
- `3d/src/draw_marks.c` — draws it.
- `3d/include/3d/shape_component.h`, `3d/src/shape_component.c` —
  `voe_physics_collider voe_3d_shape_collider(uint32_t kind)`: the collider that fits a built-in
  shape (0253 point 2); an unknown kind gets the collider's default. Header point: why the fit is
  here (the shape's size is this folder's).
- `3d/tests/collider_marker.c` (new) — a box collider's quads count 12 edges' worth; a box
  scaled 2 has its corner edge twice as far from the centre; a capsule's and a sphere's counts;
  far from the origin the quads are about the eye (small numbers). `voe_3d_shape_collider` for
  each kind. List on `3d/tests/tests.md`.
- `3d/3d.md`, `3d/src/src.md` — entries for the new files; the draw system's mentions the lines.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^3d/(collider_marker|draw_system)$'` passes.
