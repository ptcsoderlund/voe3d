# 38 — The still casters' world box
folder: 3d
after: 37
decisions: 0168, 0326, 0332

## Change
0332 point 1, for bug 03 of 051: the bounce grid will be fitted to the level, and the level is
the box of the casters that stand still. Nothing calls the new function outside its test yet;
card 42 fits the grid to it. Read the headers of `3d/src/draw_bounce.c` and `draw_bounce.h` first.
- `3d/src/draw_bounce.h`: `bool voe_3d_bounce_box(const voe_ecs_world *world, const
  voe_render_device *device, const voe_3d_frame *frame, voe_math_double3 *min,
  voe_math_double3 *max)`. Comment: the world box, in double about the world origin, of every
  caster drawn into a capture pass (meshes and the frame's model parts) that did not move
  this step; false and nothing written when none is still. A mesh whose id names nothing is
  skipped.
- `3d/src/draw_bounce.c`: it walks as `voe_3d_bounce_stale` does, with the same `moved` test,
  over exactly the casters `voe_3d_draw_casters` draws (`3d/src/draw_shadows.c`'s header: the
  frame's `hidden`, non-casting shapes and models are left out). Each still caster: its geometry's own
  box from `voe_render_geometry_box` (`render/include/render/device.h`), a model part's box in
  model space, under the entity's world transform at lag 0 from `voe_scene_transform_between`
  (`scene/include/scene/transform_system.h`): the eight corners scaled, rotated and moved in
  double, never through the eye-relative float matrix, so where the eye stands cannot change the
  box by a rounding (0332). Header paragraph: what the box is, why still casters only (a flying
  shell or a dragged box would stretch the grid), why double.
- `3d/tests/bounce.c`: a BOX case: a 40 × 0.1 × 40 ground shape at y −0.05 and a 2 m cube at
  (5, 1, 0) give (−20, −0.1, −20) to (20, 2, 20) within 1e-4, the same at frame eyes (0, 0, 0)
  and (10 km, 5, −10 km); the cube turned 45° about y widens x to 5 ± √2; a caster moved this step
  (its transform remembered, then moved) is left out, as is a shape with `cast_shadows` false and
  a hidden one; a world with no caster still: false. Take shape sizes from
  `3d/include/3d/shape_geometry.h`. Header paragraph for the case.
- `3d/src/src.md` (`draw_bounce.c`), `3d/tests/tests.md` (`bounce.c`): entries.

## Done when
`ctest --test-dir build/debug -R "^3d/bounce$"` passes.
