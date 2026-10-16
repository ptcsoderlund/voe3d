# 22 — A light that is off gives no light
folder: 3d
after: 21
decisions: 0168, 0427, 0428

## Change
0428 point 4, using `voe_scene_identity_off` (`scene/identity_component.h`)
on each light row's owner.

- `3d/src/draw_system.c`, `voe_3d_draw_system_light` (line 64; the file is
  780 lines, open only that function): when row 0's owner is off, the
  light it returns has `intensity` 0 and `fill` nought, direction and
  colour as now. Its comment says why the row is kept and not skipped
  (table order of the sky's discs, `more_lights` and shadow slots).
- `3d/src/draw_light_blockers.c`: `light_of` does the same for its row, and
  its comment points to the first's. `voe_3d_draw_system_light_blockers`'s
  row loop skips a blocker whose owner is off, before the shape is asked.
- `3d/src/draw_point_lights.c`: `voe_3d_draw_system_point_lights`'s row loop
  skips an off lamp where it skips one with no intensity; it thus takes no
  shadow slot.
- `3d/src/draw_shadows.c`: `voe_3d_draw_light_casts` (declared in
  `3d/src/draw_bounce.h`) is false for an off light's row, so no cascade is
  drawn for it; its comment in `draw_bounce.h` says so.

An off light's sky disc and glow, and its bounce, go by the existing rules
that read intensity above 0 (`render/device.h`'s sky comment, draw_shadows'
bounce test); nothing else changes for them. With every sun off the frame's
light is dark, as with no light table (0428 point 4).

The header comments of `draw_light_blockers.c` and `draw_point_lights.c`
gain "an off owner left out"; their entries in `3d/src/src.md` change only
if they list what is left out.

New test `3d/tests/lights_switched_off.c`, needing no graphics card, its
worlds built as `3d/tests/directional_lights.c` builds its own plus the
identity table (`scene/identity_system.h`), each light's entity given an
identity:
1. A sun then a moon, the sun off: `voe_3d_draw_system_light` has intensity
   0 and fill nought; the frame's `more_lights[0]` is the moon's as before;
   `voe_3d_draw_light_casts(world, 0)` false, `(world, 1)` true.
2. The sun on and the moon off: the sun as before, `more_lights[0]`
   intensity 0, `more_count` still 1.
3. Two point lamps (as `3d/tests/point_lights.c` makes them), one off: the
   frame's points hold one, the other's.
4. A light blocker (as `3d/tests/light_blockers.c` makes it), off: the
   frame's blockers count 0; on: 1.
The test includes `"../src/draw_bounce.h"` for `voe_3d_draw_light_casts`,
as `3d/tests/bounce.c` does. Its header lists the cases. Entry in `3d/tests/tests.md`, one sentence under 300 characters.

## Done when
The test `3d/lights_switched_off` passes after the folder's build, and
`3d/directional_lights`, `3d/point_lights`, `3d/light_blockers`,
`3d/shadow_lights` and `3d/no_light` still pass.
