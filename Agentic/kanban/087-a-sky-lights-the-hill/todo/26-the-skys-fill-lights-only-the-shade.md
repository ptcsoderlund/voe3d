# 26 — The sky's fill lights only the shade
folder: render
after: none
decisions: 0168, 0408

## Change
0408 points 1 and 2. Read the header of `render/shaders/lighting.slangh`, the comments above
`voe_render_fill`, `voe_render_sky_light` and `voe_render_directionals` in it, the lit path's lighting
lines in `render/shaders/draw.slang` (around its `voe_render_directionals` call), the comment above
`voe_render_sky` in `render/include/render/device.h`, and `render/tests/sky_light.c`.

- `render/shaders/lighting.slangh`:
  - `voe_render_directionals` gains an `out float sky_weight`: light 0's fill weight, the same
    `1 − smoothstep(0, VOE_FILL_FADE_REACH, reach)` `voe_render_fill` uses, at light 0's reach after
    its gate (nought reach where the gate stops it, so weight 1). One helper both use, e.g.
    `float voe_render_fill_weight(float reach)`, so the curve is written once.
  - `voe_render_sky_light` gains `float sky_weight`: its diffuse half is times `fill × sky_weight`;
    its reflection half is times neither (no longer times `fill`). Still nought unless `drawn` and
    where `voe_render_sky_reaches` fails; its early-out at fill nought goes, since the reflection
    stands at fill 0.
  - Header, the sky paragraph: the diffuse half is light 0's fill and fades with its weight; the
    reflection is whole, gated and occluded only. The two functions' comments say the same.
- `render/shaders/draw.slang`: the lit path takes `sky_weight` from `voe_render_directionals` and
  hands it to `voe_render_sky_light`. Nothing else changes; no other caller exists.
- `render/include/render/device.h`: the `voe_render_sky` comment on `fill` says it scales the sky's
  light in shade only, fading as light 0's fill does, and that the reflection does not follow it.
- `render/tests/sky_light.c`, cases changed and added on the cube's +Z face, and its header's list:
  - case 1 at fill 0: the shaded face no longer reads black; it reads at most what it reads at fill 1
    in every channel, and strictly less in blue;
  - new: the sun on the face's side (as case 2), white at intensity π, the sky drawn: the face reads
    byte for byte the same at fill 0, 0.5 and 1;
  - new: the same sun, the face turned away at fill 1 reads no brighter, as the sum of its three
    bytes, than the face in that full sun at fill 1;
  - new: the sky zeroed, light 0's fill the red (0.5, 0, 0): the face in that full sun reads byte for
    byte what it reads with fill zero;
  - case 5's "at fill 0 black" becomes case 1's fill-0 rule; its other lines stand. Case 3 (the
    metal quad) stands; if it read the reflection times `fill` below 1, it now expects it whole.
- `render/shaders/shaders.md` (`lighting.slangh` entry) and `render/tests/tests.md` (`sky_light.c`
  entry): changed where what they say changed. Each at most 300 characters.

## Done when
The tests `render/sky_light`, `render/directional_lights`, `render/blocker_kinds`, `render/unshaded`
and `render/sky` pass after the folder's build.

Human, in the editor on the hill with a sky: raise and lower the sun's fill intensity; sunlit slopes
stay as they are and only the shaded ones brighten.
