# 0412 — The sky's record holds a backdrop and each light's disc; only metal mirrors it
date: 2026-10-13
by: planner

## Decision
For 087, carrying out 0411. Every curve is written once in `render/src/sky.c` and once in
`render/shaders/sky.slangh`, agreeing within 2/255 as drawn.

1. **Record.** `voe_render_sky` becomes 96 bytes, six float4s: `zenith` + `horizon_height`, `horizon`
   + `strength`, `ground` + `haze`, `drawn` + three reserved words, `disc_size[4]`, `glow_size[4]`.
   Entry 0 of the two arrays is the pass's `light`, entry i its `more` light i − 1; entries past
   1 + `more.count` are not read. `fill`, `sunset`, `sun_size` and `sun_glow` go. Zeroed is the old
   picture, byte for byte. `voe_render_light` and `voe_render_directional_light` do not change.
2. **Backdrop.** `B(v) = strength × G(v)`, G 0407 point 4's gradient with `H = horizon` (no evening
   shift, no dims). The haze colour is B along the view with its height clamped up to 0.
3. **A light is shown** when it is not `unshaded`, its intensity is above 0 and its colour is not
   nought. A shown light adds, in its colour `c` (not times intensity, not times `strength`), the
   glow of 0407 point 5 with `glow_size` in place of `sun_glow` and no night factor; then its disc,
   when `disc_size` > 0 and the view is at or above the horizon, lays `c × DISC_GAIN` (2) over what
   is there by the disc's smoothstep edge, a lerp, not an add. So a disc's centre is `2c` whatever
   the sky, its strength or the light's intensity. Glows of all lights first, then discs in light
   order.
4. **The harmonics** are of B alone, no glow and no disc, and feed only the reflection's rough end.
5. **Lighting.** The sky adds no diffuse term and replaces no fill; light 0's reach is `N·L × shadow`
   as every light's; `sky_weight` and the lifted reach go. The sky's reflection, `lerp(sky(R), E(R)/π,
   roughness) × EnvBRDFApprox × occlusion`, gated by `voe_render_sky_reaches` as before, is also
   times the surface's metalness: a non-metal reads byte for byte the same with a sky or without.
6. **Scene.** The light row gains `disc_size` (degrees of angular radius, 0 to 10, default 1.5) and
   `glow_size` (0 to 1, default 0.3) after `bounce_strength`, in its default and unsaid rows alike;
   `VOE_SCENE_SKY_SUN_SIZE_MAX` becomes `VOE_SCENE_LIGHT_DISC_SIZE_MAX`. The sky row is `zenith`,
   `horizon`, `ground`, `horizon_height`, `strength` (at least 0, default 1), `haze`. `3d` fills
   `disc_size[i]` and `glow_size[i]` from light rows 0 to 3 in table order, as
   `voe_3d_draw_system_light` and `_lights` take them; 0 past the table.

## Reasoning
- Disc sizes in the sky record, against widening `voe_render_light`: the light record is read by
  every pass, the bounce and water; the discs only by a drawn sky, so a skyless frame is untouched.
- A disc laid over at a fixed 2c, against added: an added disc on a bright horizon goes white; laid
  over, it is the light's colour under Soft tone at any sky. 2 keeps it brighter than any backdrop
  authored at most 1.
- Glow not times `strength`: it is the light's, as the disc is; a sun in a black sky keeps its halo.
- Reflection times metalness, against for every surface: a rough dielectric's 4 % of the sky would
  change the lit hill when the sky changes, which 0411 point 1 and feature step 2 forbid; the metal
  sphere of step 8 still mirrors the sky.
- Harmonics of the backdrop only: no sun list on the CPU, and a rough metal reads a blurred sky.

## Replaces
Amends 0411 points 2, 3 and 5 with the numbers and shapes above; 0407 points 1, 3, 5, 6 and 7, 0408
and 0410 give way to it.
