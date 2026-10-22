# 05 — A cascade draws only the casters inside it
folder: 3d
after: 04
decisions: 0168, 0258, 0357, 0395, 0464

## Change
0464 point 3: each cascade skips the casters outside its sides and its far plane; its sun side is
open, so whatever casts into the view today still does, from behind the camera too.

- `3d/src/draw_bounce.h` — `voe_3d_draw_casters` gains `const struct voe_3d_cull *cull` (NULL
  draws every caster, as the point-shadow pass and the bounce want) and `voe_3d_view_counts
  *counts` (NULL counts nothing). Its comment says which callers pass which.
- `3d/src/draw_shadows.c` —
  - `voe_3d_draw_casters`: a mesh or model caster whose sphere, `voe_3d_bounce_caster_sphere` at
    the frame's lag taken about the frame's eye, the cull does not see is not drawn,
    `cast_skipped` + 1; a drawn one `cast` + 1; a landscape caster hands the cull and counts on to
    `voe_3d_draw_terrain_cast`. The blockers' `within` test is unchanged and comes first.
  - `draw_light_cascades`: for each cascade, `voe_3d_cull_of(cascades.light[c], true)` handed to
    the casters into the frame's `counts`.
  - The point-shadow pass passes NULL, NULL.
  - The file header's casters paragraph gains the cull and why the sun side is open.
- `3d/src/draw_bounce.c` — its two `voe_3d_draw_casters` calls and its terrain cast pass NULL,
  NULL: the bounce sees the level (0331), not the view.
- `3d/include/3d/draw_system.h` — `voe_3d_draw_system_shadows`'s comment: a caster is drawn into a
  cascade only when its sphere is inside the cascade's sides and far plane, counted in
  `frame->counts`; the room it asks of a device is unchanged (a ceiling).
- `3d/tests/shadow_culling.c` (new), on a headless device as `3d/tests/shadows.c` builds one: a
  ground plane, a camera looking down −Z at it, a low sun shining towards −Z:
  - a tall box 3 m behind the camera: the ground pixel where its shadow falls in view is darker
    than the same frame without the box (How to test 4);
  - a tall box 3 m to the side, outside the picture, its shadow thrown into it: the same;
  - a box 2 km to the side: `frame.counts.cast_skipped` above nought, the picture equal to the
    frame without it.
- `3d/src/src.md`, `3d/tests/tests.md` — entries follow.

## Done when
Tests `3d/shadow_culling`, `3d/shadows`, `3d/blocked_shadows`, `3d/terrain_shadow`,
`3d/directional_lights` and `3d/bounce` pass.
