# 04 — Every light blocker is lined, the selected one brighter
folder: 3d
after: 03
decisions: 0168, 0354, 0365

## Change
Breaks `editor/src/view_passes.c` (it sets `.light_blocker`); card 05
mends it. Touch nothing outside `3d`.

`3d/include/3d/draw_system.h`:
- `voe_3d_frame`'s `voe_3d_collider_marked light_blocker` becomes
  `voe_3d_rows_marked light_blockers` (the type from card 03). Comment:
  when `shown`, every blocker row with a transform is lined from
  `voe_3d_light_blocker_shape` at the frame's lag with
  `voe_3d_collider_marker_quads`; the ones not selected as one geometry in
  `colour` in the world layer inside the world's depth (with the markers);
  the `selected` one as today, after the collider's lines behind the
  outline's clear, in `selected_colour` and `material`. Pool:
  `VOE_3D_COLLIDER_MARKER_VERTICES`/`_INDICES` per blocker, two ranges and
  two objects; a pool too small draws none (0365 point 5).
- `_frame`'s and `_run`'s comments name `light_blockers` for
  `light_blocker`.

`3d/src/draw_marks.h`/`.c`:
- New `voe_3d_draw_marks_light_blockers(world, device, arena, frame)`:
  the unselected ones, gathered as `voe_3d_draw_marks_point_lights`
  gathers; nothing when not shown or the blocker table is not registered
  (`has_store`, `scene/include/scene/light_blocker_component.h`'s key).
- `voe_3d_draw_marks_light_blocker` reads `frame.light_blockers.selected`
  and draws in `light_blockers`' selected colour and material, nothing when
  not `shown`; `draw_shape_lines` takes the colour, material and pixels it
  needs rather than a `voe_3d_collider_marked` if that is simpler.
- Order list in `draw_marks.h`: the new call after the places.

`3d/src/draw_system.c`: `_frame` zeroes `light_blockers`; `_run` calls the
new function right after `voe_3d_draw_marks_places`.

`3d/tests/light_blockers.c`: `an_empty_view`/`edge_pixel`/
`the_selected_blocker_is_outlined` set `light_blockers` (shown, selected)
in place of `light_blocker`; the zeroed check reads
`frame.light_blockers.shown`. New test beside it: shown with nothing
selected, an edge pixel is the rest colour (pick one far from the outline
colour); not shown, it is the background. `CAPACITIES` grows for two
ranges.

Maps: `3d/3d.md`'s `draw_system.h` and `light_blocker.h` entries, the
`light_blocker.h` header's "ONE ANSWER" paragraph (every blocker's lines,
not the selected one's), `3d/src/src.md`'s `draw_marks` entries and
`3d/tests/tests.md`'s `light_blockers.c` entry.

## Done when
`ctest --test-dir build/debug -R '^3d/light_blockers$'` passes after a build,
and `! grep -q 'collider_marked light_blocker;' 3d/include/3d/draw_system.h`
exits 0.
