# 02 — Render bins point lights into tile and slice masks
folder: render
after: none
decisions: 0168, 0320

## Change
The record a pass will carry and the pure-CPU binning of 0320 point 5, no Vulkan, nothing drawn yet.
Read `render/src/bounce_schedule.h` and `render/tests/bounce_schedule.c` as the pattern for a pure-CPU
part and its test, and the `voe_render_view` and `voe_render_light` section of
`render/include/render/device.h`.

- `render/include/render/device.h`, beside `voe_render_light` (additions only, nothing else moves):
  - `#define VOE_RENDER_POINT_LIGHTS 256`, the most a pass carries.
  - `voe_render_point_light { voe_math_float3 position; float range; voe_math_float3 colour; float
    reserved; }`: position in the space the pass's draws place vertices in (about the eye in 3d),
    colour linear and already times strength. 32 bytes, a static_assert says so.
  - `voe_render_point_lights { const voe_render_point_light *lights; uint32_t count; }`.
  - Comment points: no shadow; ends at range with saturate(1 − (d/range)²)²; no inverse square; at
    most VOE_RENDER_POINT_LIGHTS.
- `render/src/light_bins.h` (new), header comment with a usage block like bounce_schedule.h's:
  - `VOE_RENDER_LIGHT_TILES_X` 16, `_TILES_Y` 9, `VOE_RENDER_LIGHT_SLICES` 32,
    `VOE_RENDER_LIGHT_WORDS` (VOE_RENDER_POINT_LIGHTS / 32), `VOE_RENDER_LIGHT_SLICE_NEAR` 0.1f,
    `_SLICE_FAR` 1000.0f.
  - `struct voe_render_light_bins { uint32_t tiles[16 × 9][WORDS]; uint32_t slices[32][WORDS]; }`,
    tile index x + 16 × y, x and y from NDC as (ndc + 1) / 2 × tiles; light i is bit i % 32 of word
    i / 32.
  - `uint32_t voe_render_light_slice(float distance)`: the slice of a view distance, exponential from
    NEAR to FAR, below NEAR slice 0, past FAR the last.
  - `void voe_render_light_bins_fill(const voe_render_view *view, const voe_render_point_light
    *lights, uint32_t count, struct voe_render_light_bins *out)`: zeroes `out`, then for each light
    its sphere in view space (looking down −Z): wholly behind the eye marks nothing; its slices from
    near to far side; its tiles from the NDC rectangle of the eight corners of its view-space box
    through `view->projection`, clipped to the screen, nothing when off it, every tile when the box
    reaches behind the near distance. Conservative: a tile or slice it touches is always marked, extra
    marks are allowed. Asserts count ≤ VOE_RENDER_POINT_LIGHTS.
  - Header points: why two separable masks and not cells (size, 0320 reasoning); that a fragment
    ANDs its tile's and its slice's; the projection's own NDC convention is used, so the shader
    recomputes the same NDC from the same matrices; cost is count × the tiles covered.
- `render/src/light_bins.c` (new): the above.
- `render/tests/light_bins.c` (new), including `../src/light_bins.h`: a perspective view (build its
  projection as the other render tests do); a light 10 m ahead marks the middle tiles, its slices,
  not a corner tile, not slice 0; one behind the eye marks nothing; one around the eye marks every
  tile; one far to the side marks no tile; light 40 lands in word 1 bit 8; `voe_render_light_slice`
  is 0 at NEAR, the last at FAR and past it, and rises with distance.
- `render/src/src.md`, `render/tests/tests.md`: one entry each for the new files; the device.h entry
  in `render/include/render/render.md` names point lights. Each entry at most 300 characters.

## Done when
The test `render/light_bins` passes after the folder's build.
