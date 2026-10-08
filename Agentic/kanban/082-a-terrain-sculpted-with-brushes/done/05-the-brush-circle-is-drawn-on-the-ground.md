# 05 — The brush circle is drawn on the ground
folder: 3d
after: 04
decisions: 0168, 0379

## Change
An editor's view draws the brush as two rings lying on a landscape (0379 point 3), a mark like a sun's.

- New `3d/include/3d/brush_marker.h` / `3d/src/brush_marker.c` — `VOE_3D_BRUSH_MARKER_VERTICES` and
  `_INDICES`; one call building both rings' line quads: 48 segments each, about (x, z) in the grid's own
  space at `radius` and `inner`, each point at `voe_3d_landscape_height` plus 5 cm, through the row's
  world matrix about the eye, with the same view and width inputs `3d/include/3d/sun_marker.h`'s quads
  take, through `3d/src/marker_lines.h`. Header: why lines on the ground and why two rings.
- `3d/include/3d/draw_system.h` — new `voe_3d_brush_marked`: `voe_ecs_entity entity` (zeroed for none),
  `float x, z, radius, inner`, `voe_3d_material material`, `voe_math_float3 colour`, `float pixels`,
  `voe_platform_size size`; `voe_3d_frame` gains `brush`, zeroed by `voe_3d_draw_system_frame`. Header:
  drawn with the world inside its depth after the places, one transient range and one object; an entity
  with no loaded landscape or no transform draws none; a refused range draws none.
- `3d/src/draw_marks.c` (and `draw_marks.h`'s header) — draws it in that place.
- `3d/tests/draw_markers.c` gains `brush_rings_lie_on_the_ground` (every built point's height is the
  ground's plus 5 cm) and `a_frame_with_a_brush_draws`.
- `3d/3d.md` gains the `brush_marker.h` entry and its `draw_system.h` entry names the brush; `src.md`
  and `tests.md` updated.

## Done when
`ctest --test-dir build/debug -R '^3d/draw_markers$'` passes with the two tests above.
