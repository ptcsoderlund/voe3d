# 63 — The sky record holds the haze's top
folder: render
after: none
decisions: 0168, 0418, 0419

## Change
Bug 09: the sky must be veiled by the haze layer, whose top the pass does not know. 0419 point 1:
the record gains the field; card 64 uses it. No picture changes here. Read the `voe_render_sky`
comment and struct and the pass-begin assert paragraph in `render/include/render/device.h`; the
sky struct and its comment in `render/shaders/bindings.slangh`; the sky asserts in
`render/src/records_layout.c`; the header of `render/src/pass_sky.c`.

- `render/include/render/device.h`: `voe_render_sky`'s `reserved[2]` becomes `float haze_top`
  then `uint32_t reserved[1]`, still 96 bytes. The comment gains a short `haze_top` paragraph:
  the haze layer's top in metres above the eye (the Haze row's height − the eye's world y), any
  finite, the GPU being eye-relative; what it veils is card 64's, so say only that it is the
  layer's top and `haze` 0 reads none of it. The pass-begin assert paragraph names it as finite.
- `render/shaders/bindings.slangh`: the sky's `reserved1` becomes `float haze_top`; its comment
  names it beside `haze` and `haze_start`.
- `render/src/records_layout.c`: the sky offset assert adds `haze_top` at 56; its message names it.
- `render/src/pass_sky.c`: asserts `haze_top` finite beside `haze_start`, the message naming it.
- `render/shaders/shaders.md` (`bindings.slangh`) and `render/src/src.md` (`pass_sky.c`) only if
  they list the sky's fields or checks: follow, one sentence of at most 300 characters each.

## Done when
The tests `render/sky`, `render/haze` and `render/sky_model` pass after the folder's build.
