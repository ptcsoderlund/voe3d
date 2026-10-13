# 49 — A reflection is as bright as the light on the surface
folder: render
after: 48
decisions: 0168, 0411, 0423, 0424

## Change
0424 points 1 to 3: the sky's reflection is scaled by k = saturate(max channel of L / F). Byte for byte
the old picture wherever the sky is not drawn.

- `render/shaders/lighting.slangh`:
  - `voe_render_directionals` gains `out float3 reaching`: Σ `voe_render_sun_radiance(light, reach)`
    over the lights whose gate passes, the same value its BRDF term multiplies.
  - `voe_render_points` gains `out float3 reaching`: Σ each gated light's colour × falloff × N·L ×
    visibility, the same factor its BRDF term multiplies.
  - add `float voe_render_reflection_light(float3 light)`: `light` is L; F is the largest channel of
    Σ colour × intensity over the pass's directional lights for which `voe_render_sky_shown` holds,
    at least 1; returns saturate(max channel of L / F).
  - the header's "THE SKY LIGHTS NOTHING" paragraph: the reflection's brightness is the light on the
    surface, L over F (0423, 0424); a surface no light reaches mirrors nothing. A short paragraph on L
    and F.
- `render/shaders/draw.slang`, the lit exit: take the indirect light as `voe_render_indirect` with a
  colour of one, then multiply by `base.rgb` for the indirect term (the same floats as before). L =
  directional reaching + point reaching + π × that indirect light. The sky reflection term is
  multiplied by `voe_render_reflection_light(L)`. No other entry point changes; water does not.
- `render/shaders/shaders.md`: the `lighting.slangh` entry says a metal mirrors the sky only as bright
  as the light on it, within 300 characters.
- `render/tests/sky_light.c`:
  - check 3's expected values are times k, worked out in the test from the noon sun's colour ×
    intensity × N·L on the quad (unshadowed, no fill, no bounce) over F; name k in the output.
  - a new check: the metal quad with the sun's light travelling away from its face (N·L below
    nought) and fill nought reads the same bytes as with the sky zeroed: a surface no light reaches
    mirrors nothing.
  - a new check: with the sun head-on to the quad's normal at the test's INTENSITY, the
    centre is no brighter in any channel than the sky's radiance along R plus the 6/255 tolerance.
  - the header's numbered list names both.
- `render/tests/tests.md`: the `sky_light.c` entry, at most 300 characters.

## Done when
The test `render/sky_light` passes after the folder's build, with both new checks in it.

The human's: open lumberhill_project, put Cube 2 with MetalMaterial.material in the stable's shadow,
select Sky and move strength from 0 to 1: the stable, the cube and the terrain do not change, only the
sky behind them. Move the cube into the sun: it mirrors the sky, no brighter than the sky.
