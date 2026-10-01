# 08 — 3d fits the bounce grid and its light view
folder: 3d
after: 07
decisions: 0168, 0258, 0308

## Change
0308 points 2 and 3. Read `3d/include/3d/shadow_cascades.h`,
`3d/src/shadow_cascades.c`, the bounce constants in
`render/include/render/device.h` (card 04), `3d/src/src.md`,
`3d/include/3d/3d.md`, `3d/tests/shadow_cascades.c` and `3d/tests/tests.md`.

- `3d/src/light_box.h` / `light_box.c`, new, internal, header comments:
  `basis_of`, `snap` and `look_from_the_sun` move here from
  `shadow_cascades.c`, unchanged, as `voe_3d_light_box_basis`,
  `voe_3d_light_box_snap` and `voe_3d_light_box_look` with their types; the
  header says both the cascades and the bounce grid frame the sun with them.
- `shadow_cascades.c`: calls them; its header comment points at light_box.
- `3d/include/3d/bounce_grid.h`, new, public, header comment:
  - `VOE_3D_BOUNCE_AHEAD` 32.0f (metres ahead of the eye the grid centres).
  - `typedef struct voe_3d_bounce_grid` {int32_t cell[3]; voe_math_float3
    corner; voe_render_view light;}.
  - `voe_3d_bounce_grid voe_3d_bounce_grid_fit(voe_render_view view,
    voe_math_double3 eye, voe_math_float3 direction)` — forward from the
    view; centre = eye + forward × ahead, in double; `cell` = floor(centre /
    spacing) − probes / 2 per axis, so the grid is whole cells about the
    world origin; `corner` = cell × spacing − eye, as float; `light` looks
    from the sun along `direction` at the grid's centre, an orthographic box
    of the grid's bounding sphere (radius √3 × probes × spacing / 2), depth
    reaching `VOE_3D_SHADOW_CASTER_REACH` toward the sun, snapped to whole
    8-texel blocks of `VOE_RENDER_BOUNCE_TEXELS`.
- `3d/src/bounce_grid.c`, new, header comment.
- `3d/tests/bounce_grid.c`, new, cases:
  - the grid's centre lies within one cell of eye + forward × 32;
  - eye + corner equals cell × 2 within 1 mm, far from the origin too
    (eye at 10 km);
  - moving the eye 2 m along +x with the view's forward −z moves cell x by 1;
  - every corner of the grid projects inside the light view's clip box;
  - moving the eye by 1 cm leaves the light view's projected world origin
    on the same 8-texel block (snapped).
- `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: entries
  changed or added.

## Done when
The test `3d/bounce_grid` passes, and `3d/shadow_cascades` and
`3d/shadows` still pass, after the folder's build.
