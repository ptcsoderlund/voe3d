# 0415 — Haze is lit by the directional lights, not the sky
date: 2026-10-13
by: planner

## Decision
For 087 bug 07. The sponsor: "I want haze to not get light from sky. Haze should be lit from directional
light(s)."

1. **The haze's colour is the lights'.** `H = Σ c_i × I_i / (4π)` over the pass's directional lights that
   are shown (0412 point 3: not `unshaded`, intensity above 0, colour not nought), light 0 and each `more`
   light: colour times intensity, a quarter of what a white non-metal squarely facing that light reads
   (glTF's Lambert is `c × I / π`). The constant is `VOE_RENDER_HAZE_GAIN`, 1/(4π), in `tone.slangh`.
2. **Nothing else lights it.** No fill, no shadow, no blocker gate, no point light, no bounce, and no term
   for the view against the light: the same colour in every direction. The sky's colours, horizon height
   and strength do not reach the haze; `strength` scales the backdrop only.
3. **No shown light, no haze light.** With none, `H` is 0 and far surfaces fade towards black by the same
   `f`: the haze adds no light.
4. **0414 stays.** `haze` and `haze_start` keep their meaning, their place on the sky row and in
   `voe_render_sky`, and the fade `f = 1 − 2^(−max(d − haze_start, 0)/haze)`. Only the colour it mixes
   towards changes. A zeroed sky is still no haze, the old picture byte for byte; plain draws take none.

## Reasoning
- Colour × intensity, against colour alone: the bug asks for the lights' brightness as well as colour,
  and the haze then rises and falls with the light as the lit ground does.
- 1/(4π): an isotropic medium lit by one light reads `E/(4π)` deep in, a quarter of a white Lambert surface
  facing it; distant ground fades into a haze about as bright as sunlit ground, not brighter (bug 06's
  complaint was haze lighting up the world). A gain of 1/π would wash the valley towards the light's
  colour at full strength.
- Without fill: the fill is a floor for shaded surfaces (0273, 0276); lighting the haze with it would
  brighten every far surface when an artist only meant to lift shadows.
- No direction term, against a forward lobe towards the light: a lobe brightens the ground in the light's
  azimuth, the shape bug 03 removed; a lobe is later fog work, as 0411 point 4 leaves richer fog.
- No sky row field for the haze's brightness: exposure owns brightness; one fewer knob.
- The haze stays on the sky row: it is authored there, saved there, and the bug does not move it.

## Replaces
Replaces 0411 point 4's colour ("the sky's gradient at the horizon times the sky's strength", "Lights' glows
do not tint the haze") and 0412 point 2's last sentence (the haze colour as the backdrop at the horizon), and
0409. 0414 stands.
