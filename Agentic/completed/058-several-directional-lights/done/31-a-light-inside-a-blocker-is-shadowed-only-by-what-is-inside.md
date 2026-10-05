# 31 — A light inside a blocker is shadowed only by what is inside
folder: 3d
after: 30
decisions: 0168, 0361, 0357, 0258, 0350

## Change
Decision 0361 point 2 in `3d`. Read the headers of `3d/src/draw_shadows.c`, `3d/src/draw_bounce.h`
and `3d/src/draw_light_blockers.c`, the comment of `voe_render_light_blockers_mask` in
`render/include/render/device.h` (line ~631), then `voe_3d_draw_casters`, `draw_light_cascades`
and `draw_sun_shadows` in `draw_shadows.c`, and the two `voe_3d_draw_casters` calls in
`3d/src/draw_bounce.c` (lines ~309, ~355).

- `3d/src/draw_bounce.h` and `3d/src/draw_shadows.c`: `voe_3d_draw_casters` takes a last
  `uint32_t within`. A caster is drawn only when the mask of its world matrix's origin, about the
  eye at the frame's lag as a light's place is, over `frame->blockers`' kept records, holds every
  bit of `within`. `within` 0 draws every caster as today, with no mask taken. The comment says so.
- `3d/src/draw_shadows.c`:
  - `draw_light_cascades` passes the light's own mask: `frame->blockers.sun` for light 0,
    `more_lights[light − 1].blockers` after it. A small static next to `frame_light` gives it.
  - the point-shadow pass passes 0.
  - Header: the cascades paragraph says a light a blocker holds casts only what those blockers
    hold (0361), and the point pass draws every caster.
- `3d/src/draw_bounce.c`: the capture passes pass 0; the relight's sun map for sun i passes that
  sun's mask, as above. Its header, if it describes the casters, says so.
- `3d/src/src.md`: the `draw_shadows.c` entry.
- New `3d/tests/blocked_shadows.c`, built as `3d/tests/shadow_lights.c` builds its picture and
  device (read its header), with its `3d/tests/tests.md` entry. A floor and a cube on it inside one
  blocker box, a flattened cube as a roof above it outside the box. A sun of intensity 0, then a
  moon inside the box shining straight down, both bounces 0. Floor pixels: one in the open, one
  under the cube. Cases, each on the second frame:
  1. For each Block kind, All, Direct, Fill: the moon casting gives the open floor pixel within
     2 of the moon not casting, per channel, and well above black; the pixel under the cube is
     darker than the open one.
  2. No blocker, the moon casting: the roof shadows the floor, the open pixel near black.
  3. The moon as the only light, light 0, inside a Block All box and casting: as case 1.

## Done when
`ctest --test-dir build/debug -R "^3d/(blocked_shadows|shadow_lights|shadows|bounce|light_blockers|no_light)$"`
passes after the 3d build.

The human's: in the editor, bug 02's steps 1–5 (any scene with a sun): the inside of a Block All,
Direct and Fill blocker is lit by the light inside it with Cast shadows on and off, and the thing on
the floor casts a shadow when it is on.
