# 0347 — A light blocker is a box that splits light into inside and outside
date: 2026-10-04
by: planner

## Decision
For 056, carrying out 0307 point 7:
1. **The row.** `voe_scene_light_blocker` in `scene`: `size` FLOAT3, metres along its local axes,
   default (1, 1, 1), finite and not negative; needs a transform, menu "Rendering / Light blocker",
   its whole-row intent its replace, drained by `voe_scene_light_blocker_system_run` beside the sun's.
   Placed, turned and scaled by its transform, so a parent carries it. No mesh, no collider, no shadow.
2. **The box in render.** `voe_render_light_blocker` is 64 bytes: three rows taking a position in the
   pass's space (about the eye) into the box's unit space, inside when every row's |row.xyz · p + row.w|
   is at most 1, and a bounding sphere (xyz centre, w radius) tested first so a far box costs one
   compare. `VOE_RENDER_LIGHT_BLOCKERS` 32 a pass and a world; the game world's table holds no more, so
   no blocker is ever left out.
3. **Inside and outside by mask.** A point's mask has bit i set when blocker i holds it. A surface is
   tested at its position pushed `VOE_RENDER_LIGHT_BLOCKER_PUSH` 0.05 m along its normal, so a wall's
   inner face is inside and its outer face outside. The sun's direct light and the fill reach a surface
   only when its mask is nought. A point light reaches a surface only when the light's mask equals the
   surface's, so a lamp inside lights the inside and nothing outside, and one outside nothing inside.
   A point light's mask is worked out on the CPU at pass begin.
4. **The bounce keeps the same rule.** The read weights a probe by nought unless its mask equals the
   surface's. The relight lights a texel by the sun only when the texel's mask is nought and by a lamp
   only when their masks are equal, and a texel feeds a probe only when their masks are equal. The
   relight is needed again when the blockers change, compared about the grid's corner as lamps are, so
   an eye that moves relights nothing. Capture is unchanged.
5. **Seen only when selected.** An editor view draws the selected blocker's box as the collider's box
   lines (`voe_3d_collider_marker_quads`), behind the outline's clear. The game draws nothing.
6. **No blockers is the old picture.** With a count of nought every mask is nought and every gate
   passes; the loops run zero times.

## Reasoning
- Planes or a per-light "blocked by" list: the house case is one box; a mask covers any light kind,
  the fill and the bounce with one rule.
- Test the surface point itself: a box sized to the inside puts the inner wall exactly on its face,
  so inside and outside would flicker; the push along the normal settles it.
- A probe-mask image written by the relight: cheaper per pixel but one more image and binding; the
  bounding-sphere early out makes a far blocker cost one compare, which is enough at 32.
- More than 32: a pass's light would depend on which were kept, and choosing by the eye breaks 0328.

## Replaces
Nothing. Carries out 0307 point 7.
