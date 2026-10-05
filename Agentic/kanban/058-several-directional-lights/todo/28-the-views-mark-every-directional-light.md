# 28 — The views mark every directional light
folder: editor
after: 27
decisions: 0168, 0360, 0354

## Change
Each view marks every directional light, the selected one in the outline's colour (decision 0360
points 1 and 2). Read the header of `3d/include/3d/draw_system.h` for `voe_3d_sun_marked`.

- `editor/src/view_passes.c`:
  - `first_placed_light` and the `sun_entity` / `sun_colour` locals go.
  - The frame's `.sun` is set as `.point_lights` is beside it: `shown` true, `selected` the
    selection, `colour` the gizmo's rest colour, `selected_colour` the outline's, the same
    material, pixels and size.
  - The header comment and the comment near line ~31 (`marker_colour`): every light is marked, not
    the first.
- `editor/src/view_passes.h`, `VOE_EDITOR_CAPACITIES`: a new `VOE_EDITOR_SUN_MARKERS` 16 with a
  one-line comment (decision 0360 point 2); `transient_vertices` and `_indices` take the sun
  marker's count times it; `transient_geometries` one more per view (the selected sun's own). The
  comment above it says so.
- `editor/src/src.md`: only the `view_passes` entries, if their claim changed; each under 300
  characters.

## Done when
`! grep -q first_placed_light editor/src/view_passes.c && grep -q VOE_EDITOR_SUN_MARKERS
editor/src/view_passes.h` exits 0.

The human, in the editor with `examples/sun_and_moon` open (bug 01):
1. Sun, Moon and Cave Light each show the sun's icon where they are.
2. Clicking the Moon's icon selects the Moon, and only it; clicking the Sun's selects the Sun.
3. The selected light's icon is in the outline colour, the others in the rest colour.
