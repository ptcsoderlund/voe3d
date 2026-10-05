# 27 — Every directional light is marked
folder: 3d
after: 26
decisions: 0168, 0360, 0274, 0354

## Change
A pass marks every light row with a transform, not one sun (decision 0360 point 1). Changes the
public `voe_3d_sun_marked`; the editor is card 28, and between them it does not build.

- `3d/include/3d/draw_system.h`:
  - `voe_3d_sun_marked` takes the fields of `voe_3d_point_lights_marked` (just below it): `shown`,
    `selected`, `material`, `colour`, `selected_colour`, `pixels`, `size`; `entity` goes. Its
    comment says every light with a transform is marked when `shown`, the selected one in its own
    colour, as two transient geometries, and that the pool needs VOE_3D_SUN_MARKER_VERTICES and
    _INDICES per light, two ranges and two objects, a pool too small drawing none.
  - The frame's `sun` field: "A PASS MAY MARK EVERY SUN", not one; the doc of
    `voe_3d_draw_system_frame` and the comment near line ~569 naming `frame.sun` as one entity,
    to match.
- `3d/src/draw_marks.c`, `voe_3d_draw_marks_sun`: modelled on `voe_3d_draw_marks_point_lights` in
  the same file. Walk the light rows; for each with a transform build its quads
  (`voe_3d_sun_marker_quads`) into the rest's mesh or the selected one's; draw each non-empty
  mesh as one transient geometry in its colour. Nothing when not `shown` or the world has no
  light store. Its comment and `3d/src/draw_marks.h`'s header: every sun, not a sun.
- `3d/src/draw_system.c`: only the comment near its `frame.sun` zeroing, if it says one sun.
- `3d/tests/draw_markers.c` (card 26): the sun record set as the new shape. New cases:
  - two lights at different places, `shown` with neither selected: one more draw than not
    shown; with one selected: two more;
  - those two lights: `voe_3d_pick` with a ray at each light's place answers that light and not
    the other (`voe_3d_pick_ray` or a hand-built ray; read `3d/include/3d/pick.h`'s header).
  Its header and the `draw_markers.c` entry in `3d/tests/tests.md` say so.
- `3d/src/src.md` and `3d/include/3d/3d.md`: only entries whose claim changed (the `draw_marks`
  and `draw_system.h` ones).

## Done when
`[ $(grep -c selected_colour 3d/include/3d/draw_system.h) -ge 2 ]` exits 0, and the test
`3d/draw_markers` passes with the two new cases.
