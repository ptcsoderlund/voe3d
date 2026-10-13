# 01 — The sky looks like a photograph and holds the light's settings

## Seen
"its too intense." With the hill's orange sun (colour 1, 0.42, 0.31, intensity π) about 25° up, the whole sky
is salmon and the horizon is a band brighter than white: worked out from 0405's model, the horizon under the
sun is (2.0, 0.6, 0.26) and away from it (0.49, 0.29, 0.19), against a zenith of (0.04, 0.03, 0.03), where a
white surface facing the sun is 1.0. The sky's `light` field set to 0 changes nothing in the drawn sky.
"I want games to look like games. Not photorealistic, unless its cheap." "I dont think light should be a
parameter in the sky since that is intensity in the directional light. directional light should have
settings for light and sky should have settings for sky and not light."

The editor's views drawing no sky at all was fixed on the branch in commit 584b1af0
(`voe_3d_draw_system_sky`); keep it.

## Expected
As decision 0406, which amends 0405 points 1, 2, 3 and 5:
- The sky row holds zenith, horizon and ground colours, horizon height, sun size, sun glow, sunset and haze,
  with 0406's defaults, and no light, exposure or tone.
- The sky is an authored gradient with a sun disc and glow in the light's colour, shifted towards evening by
  `sunset` as the sun lowers; nothing but the disc and glow is brighter than authored; an unshaded or dark
  light gives the plain gradient, not black. C and Slang agree within 2/255.
- With a sky, the first directional light's fill takes the sky's colours (diffuse and reflection, outside
  Indoors and Room blockers), scaled by its fill intensity; otherwise the fill is as before.
- A new Rendering / Picture row holds exposure and tone (default Soft); no row is ×1 and Clip.
- No sky row and no picture row is still the old picture, byte for byte: the tank and coin games unchanged.

## How to reproduce
1. Open lumberhill in the editor; the Light has colour (1, 0.42, 0.31), intensity π, rotated about 25° up.
2. Add Rendering / Sky to an entity, defaults unchanged.
3. Look at the horizon and the sky around the sun in a view.
