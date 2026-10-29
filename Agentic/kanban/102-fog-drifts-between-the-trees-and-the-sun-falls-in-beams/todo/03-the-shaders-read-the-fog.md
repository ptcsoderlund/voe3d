# 03 — The shaders read the fog
folder: render
after: 02
decisions: 0168, 0411, 0480, 0492, 0493

## Change
0493 points 4 and 6 as Slang functions; nothing calls them until card 04. Read
`render/shaders/shaders.md`, `render/shaders/bindings.slangh` (the block, the fog records, binding
13), the header of `render/shaders/clouds.slangh` and its `voe_render_clouds_phase` and
`voe_render_clouds_march` (the pattern), in `render/shaders/lighting.slangh` the header,
`voe_render_sunlight`, `voe_render_cloud_shadow` and `voe_render_fill`, in
`render/shaders/tone.slangh` the header and `voe_render_haze`, and the include lines of
`render/shaders/draw.slang`.

- `render/shaders/lighting.slangh`: `float voe_render_sunlight_at(voe_render_shadow shadow,
  float3 world)`: what reaches a point in the air from a light's cascades, one compare, the
  cascade chosen as `voe_render_sunlight` chooses it, no push along a normal, 1 past the last
  cascade or with no shadow. The header's cascade paragraph names it.
- `render/shaders/fog.slangh` (new), a part, all in eye-relative metres, reading the block's `fog`;
  with count 0 every read is empty.
  - `float2 voe_render_fog_crossing(voe_render_fog_volume volume, float3 direction)`: the
    distances along a unit view at which it enters and leaves the box, by the rows in unit space;
    empty when it misses.
  - `float2 voe_render_fog_density(float3 at)`: the density summed over the volumes holding `at`,
    each as 0493 point 4 (the detail volume at `patches` + at over PATCHES, eroded by it at
    `change` + at over CHANGE, the edge fade, lerp by patchiness), and the density-weighted mean of
    their `godrays`.
  - `float3 voe_render_fog_light(float3 at, float3 view, float godrays)`: the light a unit of fog
    sends along `view`, as 0493 point 6: the first light by the half-isotropic phase times
    lerp(1, `voe_render_sunlight_at` × `voe_render_cloud_shadow`, godrays), plus every shown
    light's fill as a white surface's floor takes it; never the sky.
  - `voe_render_fog_marched voe_render_fog_march(float3 direction, float reach, float jitter)`:
    premultiplied light, transmittance and the mean distance the light came from; steps the union
    of the view's crossings of every volume within `reach`, at most 32 steps shared by length, the
    first offset by `jitter`, stopping under 0.01 transmittance; no crossing is light 0 and
    transmittance 1 with no step taken.
  The header: what each answers, why the detail volume and no texture of its own, why godrays is
  a lerp towards the shadowed light (the fog stays as its beams fade), why the first light alone,
  that blockers do not gate it, and the cost in reads a step.
- `render/shaders/draw.slang`: includes `fog.slangh` after both `lighting.slangh` and
  `tone.slangh`; the include list's comment names it.
- `render/shaders/shaders.md`: an entry for `fog.slangh`; `lighting.slangh`'s and `draw.slang`'s
  follow; each at most 300 characters.

## Done when
The folder's build compiles `draw.slang` with the new part, and the tests `render/sky`,
`render/shadow` and `render/directional_lights` pass after it.
