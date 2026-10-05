# 12 — Each casting light casts its own cascades
folder: 3d
after: 11
decisions: 0168, 0357, 0258, 0324

## Change
Decision 0357 point 3 in `3d`: every casting directional light gets a slot and four cascades. Read
the header of `3d/src/draw_shadows.c`, then its `draw_sun_shadows`, `voe_3d_draw_light_casts` and
`anything_bounces`.

- `3d/src/draw_bounce.h` and `3d/src/draw_shadows.c`: `voe_3d_draw_light_casts(world)` becomes
  `voe_3d_draw_light_casts(world, uint32_t light)`, light row `light` in table order, false past
  the count. Update its one caller in `3d/src/draw_bounce.c` to pass 0.
- `3d/src/draw_shadows.c`:
  - remove the at-most-one-light asserts. `anything_bounces` still reads row 0; card 13 widens it.
  - `draw_sun_shadows` becomes a loop over light 0 (`frame->light`, `frame->shadow`) and each
    `frame->more_lights[i]` (row i + 1). A light casts when it is shaded, of some strength, the
    frame is not blind and its row casts.
  - First it calls `voe_render_shadow_lights_ready(device, casting)` once, with the count of
    casting lights. Slots go to the casting lights in table order, up to what is ready.
  - Each slotted light fits its own cascades to its direction, and opens four passes at layer
    `slot × 4 + cascade`. Its shadow record takes the cascades' and `slot`. A light with no slot,
    or that does not cast, keeps a zeroed record.
  - On failure every record is zeroed.
  - Header: the cascades point says every casting light, its slot, and that a light past what is
    ready is unshadowed that frame.
- `3d/include/3d/draw_system.h`: the `voe_3d_draw_system_shadows` comment says each casting light
  of the frame casts, 4 passes and 4 × the drawn objects per casting light, and grows the device's
  array the first frame it is short.
- `3d/src/src.md`: the `draw_shadows.c` entry.
- New `3d/tests/shadow_lights.c`, built as `3d/tests/shadows.c` builds its picture (read its
  header), with its `3d/tests/tests.md` entry. Cases:
  1. A sun straight down, casting, and a moon shining down and toward +x at 45°, casting. On the
     second frame the floor shows two shadows, the sun's under the cube and the moon's to +x of
     it, each darker than the lit floor. The device's capacities fit two lights.
  2. The sun's `cast_shadows` false: the moon's shadow stays and the one under the cube is gone.
  3. Only the sun casts, the moon not: on a device whose `passes` are the one light's four
     cascades and the view, as `3d/tests/bounce.c` budgets its passes (read its header), the
     call returns true, and `voe_render_shadow_lights_ready(device, 0)` stays 1.

## Done when
`ctest --test-dir build/debug -R "^3d/(shadow_lights|shadows|bounce|no_light)$"` passes after the 3d
build.
