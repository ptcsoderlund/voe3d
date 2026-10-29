# 07 — The clouds' march and composite shaders and pipelines
folder: render
after: 06
decisions: 0168, 0418, 0492

## Change
0492 point 5's two stages as entry points and pipelines; card 08 records them. Read
`render/shaders/draw_sky.slangh` (the pattern; its vertex stage is reused),
`render/shaders/clouds.slangh`, `render/shaders/tone.slangh`, the end of
`render/shaders/draw.slang` where its parts are included, the header of `render/src/pipeline.c`,
the prepare comment above `voe_render_device_prepare_steps` in `render/include/render/device.h`,
and `render/tests/prepare.c`.

- `render/shaders/tone.slangh`: the sky's veil gains a form that stops at a distance: the fade
  over the part of the view's path below `haze_top` and nearer than that distance; the endless
  form is unchanged, byte for byte.
- `render/shaders/draw_clouds.slangh` (new), included by `draw.slang` after `draw_sky.slangh`; both
  stages run behind `voe_render_sky_vertex`.
  - `voe_render_clouds_march_fragment`: drawn at half size; reads the pass's depth copy at the
    2 × 2 pixels under it, takes the furthest as the reach along its view (no geometry is no
    limit), jitter from interleaved gradient noise of the pixel, `voe_render_clouds_march`, then
    the light veiled by the haze up to the march's distance as the sky is (0418 point 2); writes
    premultiplied light and transmittance.
  - `voe_render_clouds_composite_fragment`: at full size; a pixel whose own depth is nearer than
    `voe_render_clouds_entry` along its view takes nothing; else the clouds image at its place,
    bilinear, its straight colour exposed and toned as a blended mesh exit, out premultiplied
    with alpha 1 − transmittance. The clouds image is the bindless texture `clouds_image` names.
  The header: why half size and the furthest depth, why the skip, the veil, a part never
  compiled alone.
- `render/src/pipeline.c`: two kinds after the sky's, built by prepare like the others: clouds
  march (the sky's vertex stage, the march fragment, one RGBA16F colour attachment, no depth, no
  blend, nothing culled) and clouds composite (the sky's vertex stage, the composite fragment, the
  target's formats, no depth test or write, premultiplied blend, nothing culled). The header's
  count and list follow; prepare's steps rise by two (six, nine with shaderOutputLayer).
- `render/src/device_internal.h`: the device's pipeline handles for both, beside the sky's.
- `render/include/render/device.h`: the prepare comment names the two and the new counts.
- `render/tests/prepare.c`: its expected step counts and comment follow.
- `render/shaders/shaders.md` (`draw_clouds.slangh`, `tone.slangh`, `draw.slang`),
  `render/src/src.md` (`pipeline.c`): follow, each at most 300 characters.

## Done when
The tests `render/prepare`, `render/haze_sky` and `render/sky` pass after the folder's build.
