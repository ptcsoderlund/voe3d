# 08 — The shadows call draws the casters into the point-shadow pass
folder: 3d
after: 07
decisions: 0168, 0324, 0325

## Change
0325 point 6. The call now reads `frame->points`, so the loop fills them first; game and editor
are reordered by cards 09 and 10, and until then draw with no lamp shadow, which builds.
- `3d/include/3d/draw_system.h`: `#define VOE_3D_POINT_SHADOW_TEXELS 256u`, the
  `point_shadow_size` a device is given. The file header's example and order: point lights, then
  shadows, then the pass. `voe_3d_draw_system_shadows`'s comment: after the cascades and bounce
  (still only when the sun casts) it opens one point-shadow pass when
  `voe_render_point_shadows_ready` and a light in `frame->points` has a slot, and draws the same
  casters into it; one more pass and at most one object per caster; false as before when render
  refuses. `voe_3d_frame`'s `points` comment: filled before this call.
- `3d/src/draw_shadows.c`: the early return for a light that does not cast covers the sun's
  passes only; then the point-shadow pass, `voe_3d_draw_casters` into it, its end. Header: who
  casts for a lamp (the same casters, 0324 point 5) and why the walk is shared.
- `3d/src/src.md`: `draw_shadows.c`'s entry.
- `3d/tests/shadows.c`: its device gets `point_shadow_size` `VOE_3D_POINT_SHADOW_TEXELS` and one
  more pass. New case, a world whose light does not cast, a floor, a cube at the origin and a
  lamp (`cast_shadows` true, range 6) at (−1.5, 1.5, 0); point lights, shadows, the pass, read back:
  the floor at x = +1.5 darker than with the lamp's `cast_shadows` false, the same as it with the
  cube's shape `cast_shadows` false; the lamp at (+1.5, 1.5, 0) darkens x = −1.5 instead.
  Entry in `3d/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R "^3d/shadows$"` passes.
