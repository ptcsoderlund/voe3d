# 06 — The shaders read the cloud field
folder: render
after: 05
decisions: 0168, 0411, 0480, 0492

## Change
0492 points 4, 6, 7 and 8 as Slang functions; nothing calls them until cards 07, 09 and 10.
Read `render/shaders/shaders.md`, `render/shaders/bindings.slangh` (the block, `voe_render_clouds`,
bindings 13 and 14), `render/shaders/sky.slangh` (whether a light is shown), the header and the
directional-light part of `render/shaders/lighting.slangh`, and `render/src/clouds_noise.h`.

- `render/shaders/clouds.slangh` (new), a part including `sky.slangh`, all in eye-relative
  metres, reading the pass block's `clouds` and lights; with kind none every read is empty.
  - `voe_render_clouds_density(float3 at, bool detail)`: the field point is `origin` plus
    `at.xz`, the weather at it over `VOE_RENDER_CLOUDS_PERIOD` taken by `coverage` to a base
    density (0 none, 1 everywhere on the layer); Volumetric shaped by a rounded height profile
    over `bottom` to `bottom + thickness` and, with `detail`, eroded by the detail volume at the
    field point over 256 m; Flat is meant only at the plane `bottom`.
  - `voe_render_clouds_entry(float3 direction)`: the distance at which a view first meets the
    layer, infinite when it never does; a march and the composite's skip use it.
  - `voe_render_clouds_transmittance(float3 at, float3 to_light)`: how much of a light reaches
    `at` through the shape without detail, four samples over the crossing (Volumetric) or the
    plane's opacity (Flat); 1 when `at` is above the layer or `to_light.y` ≤ 0.
  - `voe_render_clouds_light(float3 at, float3 view, float density)`: the light a unit of cloud
    sends along `view`, each shown light's colour × intensity by the phase, towards the first
    light through three samples of the shape, towards the others by the depth below the top, the
    shade from the lights' fills and the many-times-scattered share; never the sky.
  - `voe_render_clouds_march(float3 direction, float reach, float jitter)`: a struct of light
    (premultiplied), transmittance and the distance the light came from on average; Volumetric
    steps the layer's crossing within `reach` and 40 km, at most 64 steps, the first offset by
    `jitter`, stopping under 0.01 transmittance; Flat one sample at the plane.
  - `voe_render_clouds_reflected(float3 direction)`: premultiplied light and opacity of one
    sample where `direction` meets the layer's middle, lit as `_light`.
  The header: what each answers, that the march, shadows and reflections read one field so they
  agree (0480 point 2), the sky lights nothing (0411), and the costs in reads.
- `render/shaders/lighting.slangh`: includes `clouds.slangh` beside `sky.slangh`; nothing else.
- `render/shaders/shaders.md`: an entry for `clouds.slangh`, at most 300 characters.

## Done when
The folder's build compiles `draw.slang` with the new part, and the tests `render/sky` and
`render/directional_lights` pass after it.
