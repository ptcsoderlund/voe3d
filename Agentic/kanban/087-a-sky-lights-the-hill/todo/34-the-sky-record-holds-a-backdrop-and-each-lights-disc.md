# 34 — The sky record holds a backdrop and each light's disc
folder: render
after: 33
decisions: 0168, 0411, 0412

## Change
0412 points 1 to 4 in C and Slang together. `3d` breaks until card 38; do not touch it. Read the
headers of every file below and the `voe_render_sky` comment and the pass-begin assert paragraph
in `render/include/render/device.h`.

- `render/include/render/device.h`: `voe_render_sky` laid out as 0412 point 1, static_assert 96;
  its comment says what each field means and bounds it (`strength` at least 0, `disc_size` 0 to
  10, `glow_size` 0 to 1, entry i is light i), that the sky lights nothing, and the shown rule.
  `voe_render_pass_begin`'s assert paragraph names the new bounds.
- `render/shaders/bindings.slangh`: the twin struct (the two arrays as float4s) and its comment.
- `render/src/records_layout.c`, `render/src/device_parts.h`: the frame block's size and offset
  asserts follow; the `sky_harmonics` comment says backdrop only.
- `render/src/sky.h`, `render/src/sky.c`: WARM, the evening and night constants and
  `voe_render_sky_sun` go; `VOE_RENDER_SKY_DISC_GAIN` 2. Signatures:
  - `bool voe_render_sky_shown(voe_render_light light)`: 0412 point 3's rule;
  - `voe_math_float3 voe_render_sky_gradient(const voe_render_sky *sky, voe_math_float3 view)`:
    the backdrop, strength included;
  - `voe_math_float3 voe_render_sky_radiance(const voe_render_sky *sky, const voe_render_light
    *lights, uint32_t count, voe_math_float3 view)`: lights[i] is entry i, count at most
    VOE_RENDER_DIRECTIONAL_LIGHTS; backdrop, every shown light's glow, then discs laid over;
  - `void voe_render_sky_harmonics(const voe_render_sky *sky, voe_math_float4 out[9])`;
    irradiance unchanged.
  Header: the model in phrases (backdrop, shown, glows then discs, disc fixed at 2c), the twin rule.
- `render/shaders/sky.slangh`: the same arithmetic; its radiance takes only `float3 view` and reads
  the frame block's sky, `light` and `more[0 .. more_count)`; the gradient takes the sky and view.
- `render/src/pass_sky.c`: validates the new fields; harmonics whenever `drawn`.
- Callers: `render/shaders/draw.slang`'s sky fragment, `render/shaders/tone.slangh`'s haze (the
  gradient, so × strength) and `render/shaders/lighting.slangh`'s reflection take the new calls.
- Tests, only so they build and their claims hold: `render/tests/sky_model.c`, `sky.c`, `haze.c`,
  `sky_light.c` move to the new fields and calls, with `disc_size[0]` and `glow_size[0]` where
  `sun_size` and `sun_glow` were and `strength` 1; cases on sunset or evening go, with their header
  lines; a "dark sun" case reads the plain backdrop. Card 35 adds the new claims.
- `render/src/src.md` (`sky.h`, `sky.c`, `pass_sky.c`), `render/shaders/shaders.md` (`sky.slangh`,
  `tone.slangh`, `bindings.slangh`), `render/tests/tests.md` (any entry naming sunset): changed
  where what they say changed, each one sentence of at most 300 characters.

## Done when
The tests `render/sky_model`, `render/sky`, `render/haze`, `render/sky_light`, `render/exposure`
and `render/directional_lights` pass after the folder's build.
