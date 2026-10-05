# 13 — Every bouncing sun bounces in 3d
folder: 3d
after: 12
decisions: 0168, 0357, 0326, 0329, 0330

## Change
Decision 0357 points 1 and 4 in `3d`: the bounce begin carries the further suns, and each
casting one draws its own relight sun map. Read the header of `3d/src/draw_bounce.h`, then
`voe_3d_draw_bounce` and `draw_sun_map` in `3d/src/draw_bounce.c`, and `anything_bounces` in
`3d/src/draw_shadows.c`.

- `3d/src/draw_bounce.c`:
  - the bounce frame's `more` is the frame's further lights. Their `bounces` and
    `bounce_strength` already come from their rows (card 11).
  - The first sun's bounces and strength come from row 0 whenever there is a row; the
    `count == 1` test goes.
  - `draw_sun_map` takes the sun's index and direction. It is called for sun 0 when row 0 casts,
    and for sun i + 1 when row i + 1 casts, each with `voe_3d_bounce_grid_sun` at its own
    direction.
  - Header: the bounce takes every sun.
- `3d/src/draw_bounce.h`: `voe_3d_draw_bounce`'s comment says one bounce shadow pass per casting
  sun.
- `3d/src/draw_shadows.c`: `anything_bounces` is true too when a further light has bounces of 1 or
  more, intensity above nought and is shaded. Its header point on who bounces names them.
- `3d/include/3d/draw_system.h`: the probe-bounce paragraph of `voe_3d_draw_system_shadows` says
  every sun, and one more pass and one more object per caster for each casting sun that bounces.
- `3d/src/src.md`: the `draw_bounce.c` entry.
- `3d/tests/shadow_lights.c` (card 12's): new cases, added to its header:
  4. A first sun of intensity 0 and bounces 0, and a moon with bounces 1 and intensity 3. A floor
     pixel in the moon's shadow, read on the frame after the volume builds, is brighter than
     with the moon's bounces at 0.
  5. With the sun and the moon both bouncing and casting, on a device whose passes are one short
     of the two bounce shadow passes, the call is false. With one more pass it is true.

## Done when
`ctest --test-dir build/debug -R "^3d/(shadow_lights|bounce|bounce_scene|shadows)$"` passes after the
3d build.
