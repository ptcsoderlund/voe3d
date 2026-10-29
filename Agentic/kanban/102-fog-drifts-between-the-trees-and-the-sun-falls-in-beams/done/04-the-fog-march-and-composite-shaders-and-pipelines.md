# 04 — The fog's march and composite shaders and pipelines
folder: render
after: 03
decisions: 0168, 0492, 0493

## Change
0493 point 5's two stages as entry points and pipelines; card 05 records them. Read
`render/shaders/draw_clouds.slangh` (the pattern: its depth reach, jitter, upsample and both
stages), `render/shaders/fog.slangh` (card 03), `voe_render_haze` in `render/shaders/tone.slangh`,
the include lines at the end of `render/shaders/draw.slang`, the header of `render/src/pipeline.c`
and its clouds kinds, the clouds pipeline handles in `render/src/device_internal.h`, the prepare
comment above `voe_render_device_prepare_steps` in `render/include/render/device.h`, and
`render/tests/prepare.c`.

- `render/shaders/draw_fog.slangh` (new), included by `draw.slang` after `draw_clouds.slangh`;
  both stages run behind `voe_render_sky_vertex`.
  - `voe_render_fog_march_fragment`: drawn at half size; reads the pass's depth copy at the 2 × 2
    pixels under it and takes the checkerboard's choice (x + y even the nearest, odd the
    furthest; the clear is no limit, as is no depth copy), jitter from interleaved gradient noise,
    `voe_render_fog_march`, then its straight light hazed as a surface at the mean distance along
    the view is, weighted back by opacity so it stays premultiplied; writes premultiplied light
    and transmittance.
  - `voe_render_fog_composite_fragment`: at full size; the four nearest half-size texels, each
    weighted bilinearly and by how near the distance its march chose (worked out again from the
    depth copy as the march chose it) is to the pixel's own, normalised, the nearest in distance
    alone when every weight is negligible; its straight colour exposed and toned as a blended mesh
    exit, out premultiplied with alpha 1 − transmittance. The image is the bindless texture
    `clouds_image` names, read by Loads as the clouds' upsample is.
  The header: half size and the checkerboard, why depth-weighted and not plain bilinear (no halo
  round a trunk), the haze, a part never compiled alone.
- `render/src/pipeline.c`: two kinds after the clouds', built by prepare like them: fog march
  (the sky's vertex stage, the march fragment, one RGBA16F colour attachment, no depth, no blend,
  nothing culled) and fog composite (the sky's vertex stage, the composite fragment, the target's
  formats, no depth test or write, premultiplied blend, nothing culled). The header's count and
  list follow; prepare's steps rise by two.
- `render/src/device_internal.h`: the device's pipeline handles for both, beside the clouds'.
- `render/include/render/device.h`: the prepare comment names the two and the new counts.
- `render/tests/prepare.c`: its expected step counts and comment follow.
- `render/shaders/shaders.md` (`draw_fog.slangh`, `draw.slang`), `render/src/src.md`
  (`pipeline.c`), `render/render.md` (its pipeline count): follow, each at most 300 characters.

## Done when
The tests `render/prepare`, `render/clouds` and `render/sky` pass after the folder's build.
