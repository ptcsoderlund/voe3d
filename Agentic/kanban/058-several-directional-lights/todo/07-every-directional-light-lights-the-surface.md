# 07 — Every directional light lights the surface
folder: render
after: 06
decisions: 0168, 0357, 0258, 0276, 0350

## Change
Decision 0357 point 2 in the shaders. Read the headers of `render/shaders/lighting.slangh`,
`render/shaders/blockers.slangh`, `render/shaders/water.slangh` and `render/shaders/draw.slang`,
then the functions named below in `lighting.slangh`.

- `render/shaders/lighting.slangh`: the sun's functions take which light they are for, a light
  record, its shadow record and its blocker mask. Index 0 is the frame block's `light`, `shadow`
  and the region's `sun`; index i is `more[i − 1]`:
  - `voe_render_sunlight` takes the shadow record and reads layer `slot × 4 + cascade`;
  - `voe_render_sun_passes` and `voe_render_fill_reaches` take the light's own mask and segment
    end;
  - `voe_render_sun_radiance` takes the light;
  - `voe_render_indirect`: the floor is `Σ` over the lights of each fill × its own fade by its own
    reach and gate, and `max(E/π, that)` as before.

  The lit path loops over `1 + more_count` lights, a fixed loop of `VOE_RENDER_DIRECTIONAL_LIGHTS`
  with an early break, and sums the direct terms. A pass with `more_count` nought must compute
  what it does today. Header: the "one directional light" point becomes several lights, each
  gated, shadowed and filled by its own, and the cascades point names the layer.
- `render/shaders/water.slangh`: if it calls the sun's functions, it takes light 0 only, as
  before. Its header says so.
- `render/shaders/draw.slang`: change only what the new signatures need.
- `render/shaders/shaders.md`: the `lighting.slangh` entry says every light.
- New `render/tests/directional_lights.c`, built as `render/tests/blocked_light.c` builds its device
  and reads back. Read that header. It has a ground quad from above, a box caster, and a sun from
  +x with fill 0.1, and these cases:
  1. `more` empty: the reference picture.
  2. The same with a zeroed `more` array of count 0: byte for byte the reference.
  3. A faint blue moon from −x, fill 0.05: every pixel at least the reference, blue up, red
     unchanged in the box's shadow under the sun.
  4. Both shadowed, the moon on slot 1, after `voe_render_shadow_lights_ready(device, 2)` has had a
     frame: two dark patches, on opposite sides of the box.
  5. A Room blocker over the left half, the sun's mask 0 and the moon's the Room's bit: the left lit
     only by the moon, the right only by the sun.
- `render/tests/tests.md`: its entry.

## Done when
`ctest --test-dir build/debug -R "^render/(directional_lights|blocked_light|shadow|water)$"`
passes after the render build.
