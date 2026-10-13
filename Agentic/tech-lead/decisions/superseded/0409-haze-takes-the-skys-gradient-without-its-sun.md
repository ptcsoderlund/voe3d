# 0409 — Haze takes the sky's gradient without its sun
date: 2026-10-12
by: planner

## Decision
For 087 bug 03.

1. The haze's colour is the sky's evening-shifted gradient (0407 points 3 and 4) along the view with
   its height clamped up to the horizon: no disc and no glow. The sun's evening tint of the horizon
   stays, so far ground still warms on the sun's side at sunset.
2. The gradient is its own function, `voe_render_sky_gradient`, written once in `render/src/sky.c` and
   once in `render/shaders/sky.slangh` with the radiance's signature; `voe_render_sky_radiance` is
   that gradient plus the glow and disc of 0407 point 5, so the drawn sky is unchanged.
3. The harmonics and the sky's reflection are unchanged.

## Reasoning
Clamping a ground view's height to the horizon puts a low sun's disc in the haze colour of every point
in its azimuth, at every distance: a column of the disc's width down the ground. The glow under a low
sun does the same in a wider, softer band. The human wants disc and glow in the sky only; haze is
distance, not the sun. Splitting the gradient out keeps one copy of each curve.

## Replaces
Amends 0407 point 6: "haze reads the sky with h clamped up to 0, disc included" becomes the gradient
alone, h clamped up to 0.
