# 0407 — The sky's curves, and the records that carry the sky and the picture
date: 2026-10-12
by: planner

## Decision
For 087 bug 01, carrying out 0406 points 2 to 4. Every curve here is written once in `render/src/sky.c`
and once in `render/shaders/sky.slangh`.

1. **Records.** `voe_render_sky` becomes 64 bytes as four float4s: `zenith` + `horizon_height`,
   `horizon` + `sun_size` (degrees), `ground` + `sun_glow`, then `drawn` (uint), `fill`, `haze`,
   `sunset`. `fill` is the first directional light's `fill_intensity`, set by `3d`. A new
   `voe_render_exposure { float exposure; uint32_t tone; uint32_t reserved[2]; }`, 16 bytes, the
   picture row as a pass takes it; `voe_render_pass_camera` and the frame block carry it after `sky`.
   (`voe_render_picture` is already the read-back image's name.) Zeroed, both are the old picture.
2. **Inputs.** `s` the unit direction towards the sun (−the light's direction), `e = s.y`; `v` the unit
   view, `h = v.y`; `c` the light's colour, not times its intensity. The sun is *on* unless the light
   is unshaded, of intensity 0 or `c` is nought; off, `k = n = 0` and there is no disc and no glow.
3. **Evening.** `k = sunset × saturate((0.3 − e) / 0.3)`, `n = sunset × saturate(−e / 0.2)`; sun side
   `w = (0.5 + 0.5·dot(v, s))²`. The horizon colour used is `H = lerp(horizon, WARM × max(horizon.r,
   horizon.g, horizon.b), k·w)`, WARM (1.0, 0.38, 0.12): no channel brighter than the authored horizon's
   brightest.
4. **Gradient.** `h ≥ 0`: `lerp(H, zenith, pow(h, 4·horizon_height + 1/16))`; `h < 0`:
   `lerp(H, ground, saturate(−10·h))`. Then `× (1 − 0.4·k) × (1 − 0.9·n)`.
5. **Glow and disc,** added in `c` when on: glow `sun_glow × saturate(dot(v, s))^(256 − 252·sun_glow)
   × (1 − n)`; disc, when `sun_size > 0` and `h ≥ 0`, `4 × smoothstep(cos(1.1·r), cos(r), dot(v, s))
   × (1 − n)`, `r` the size in radians. These are the only terms brighter than authored.
6. **The harmonics** are of the sky without its disc, glow included; haze reads the sky with `h`
   clamped up to 0, disc included as before.
7. **The fill.** With `drawn`, light 0's fill floor under the bounce is left out wherever the sky's gate
   passes (no Indoors or Room bit outside the walls' mask), and the sky term (0405 point 5's diffuse
   and reflection) is added there times `fill`; where the gate fails, light 0's fill is as before. With
   `drawn` 0 the sky term is nought and every fill is as before.

## Reasoning
A pow, a lerp and two smoothsteps per pixel: cheaper than 0405's exponentials, and each setting moves one
thing the human can name. `horizon_height` as an exponent gives 0406's endpoints (a hard band at 0,
horizon almost to the top at 1) with one pow. A fixed warm tint scaled by the authored horizon keeps
evening from brightening the sky. Light 0's fill is replaced rather than added to, so the fill's strength
is one number whichever is lighting.

## Replaces
Nothing; completes 0406 points 2 to 4.
