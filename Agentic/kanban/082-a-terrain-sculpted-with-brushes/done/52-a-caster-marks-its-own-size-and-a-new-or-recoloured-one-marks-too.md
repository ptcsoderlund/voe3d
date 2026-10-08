# 52 — A caster marks its own size, and a new or recoloured one marks too
folder: 3d
after: 49, 51
decisions: 0168, 0387, 0389

## Change
0389 point 7, in 3d. Render already adds the reach (card 42), so a sphere here is the caster's own size.

- `3d/src/draw_bounce.h`, `voe_3d_bounce_stale`:
  - It loses its `spacing` parameter.
  - Each sphere's w is the caster's world bounding radius: half the diagonal of its world box, computed the
    way `voe_3d_bounce_box` computes a box.
  - A caster marks two spheres, at lag 1 and lag 0, when position, rotation or scale differ between them.
    Position is within a millimetre; rotation and scale are within 1e-4.
  - It marks one sphere, where it is, when the world has a previous table and
    `voe_scene_transform_remembered` is false (a new caster).
  - It marks one sphere when `voe_3d_shape_changed` is true for the entity (recoloured or reshaped).
  - Update the comment.
- `3d/src/draw_bounce.c`: the above, and the one call that passed a spacing.
- `3d/include/3d/draw_system.h`:
  - Remove `VOE_3D_BOUNCE_REACH` and its comment.
  - In the shadows paragraph (near line 616), stale spheres are a caster's own size, marked on any move,
    when new, and when the shape system changed it. A world without a previous table marks only
    recolours. A removed caster marks nothing yet (0389).
- `3d/tests/bounce.c`:
  - Rewrite the stale-sphere paragraph of its header, and its cases, to the new radius. The wall's radius
    is half its world box's diagonal.
  - Drop the spacing-of-8 m case, which no longer exists.
  - New cases:
    - `a_scaled_caster_marks`
    - `a_new_caster_marks_where_it_is`
    - `a_recoloured_shape_marks_where_it_is`: this world registers `voe_3d_shape_changes_register`.
    - `a_world_without_a_previous_table_marks_no_move`
- `3d/tests/tests.md`: the `bounce.c` entry matches.
- `3d/src/src.md`: the `draw_bounce` entries if they say "reach".

## Done when
`ctest --test-dir build/debug -R '^3d/bounce$'` passes with the four new cases, and
`grep -rn VOE_3D_BOUNCE_REACH 3d` prints nothing.
