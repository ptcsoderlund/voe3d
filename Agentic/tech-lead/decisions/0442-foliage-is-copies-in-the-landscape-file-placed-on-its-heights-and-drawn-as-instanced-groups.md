# 0442 — Foliage is copies in the landscape file, placed on its heights and drawn as instanced groups
date: 2026-10-17
by: planner

## Decision
For 090.

1. **Stored in the `.landscape`.** `[Foliage]` names up to 16 kinds, `kind0=`… quoted `.glb`
   paths read to the first missing key. `[Copies]` holds up to 65536 copies, key `c<i>` for the
   i-th in file order, value six whole numbers: kind index, x and z in millimetres in the grid's
   own space, turn in 1/65536 of a turn about +Y, scale in thousandths (1–65535), lean 0–255.
   No height is stored. Saved, reverted, followed on rename and cooked with the heights.
2. **Placed on the ground.** A copy stands at (x, height(x, z), z), its up tilted from +Y toward
   the ground's normal (central differences one cell apart) by lean / 255, then turned, then
   scaled uniformly. The store keeps each copy's placed transform and places again the copies
   over any heights a brush or an undo changes, so sculpting keeps them standing.
3. **The brush.** Scatter and Erase are brush kinds beside the sculpt kinds and Paint, sharing
   Radius and Softness, with picked kinds, Density (copies per 100 m², 0.1–50, default 4), Size
   vary (0–100 %, default 20), Turn vary (0–180°, default 180) and Lean (0–100 %, default 0).
   A scatter stamp tries at most 256 points uniform in the disc, ceil(density / 100 × πr² ×
   min(1, 4s)) of them, s clamped to 0.1; one is kept where the soft weight beats a random
   draw and no copy lies within 0.84 × √(100 / density) m, so a held brush fills to about the
   density and stops. Kind uniform among the picked, scale 1 ± vary, turn ± vary, from a
   xorshift the caller keeps. Erase removes every copy of a picked kind within the radius.
4. **One stroke is one undo step** carrying the copies it added or removed, put back by value.
5. **Drawn instanced** (0375 point 3). Once a frame per view, before the shadows, every copy of
   every drawn landscape row is given its level by 0425's choice and written as an instance,
   once, into its group of (kind, level, part); a copy inside a dissolve band is also an
   instance of the level before with the other dither, in a group of its own. Each group is one
   instanced draw in the camera pass, inside a `foliage` span, and in each cascade of a light no
   blocker holds, the band's extra groups left out there. Not in bounce captures, sun maps or
   point shadows, whose instance index counts faces; not blended parts; no culling (093).
6. **Render's instance** is 64 bytes: the top three rows of the camera-relative world matrix,
   the dither, three reserved words. It replaces the object record's world and dither; its
   normal matrix is worked out in the shader, exact for a turn and a uniform scale. The record's
   `reserved0` becomes `instances`, the first index plus one. A device's `instances` capacity
   is per frame slot and may be nought.

## Reasoning
- A `.foliage` file beside the landscape: another kind to make, follow, save and cook, when the
  `.landscape` already rides through every one of those paths (0379, 0402).
- Storing each copy's height: a raised ground would bury or float it (feature step 4).
- A whole object record per copy: 272 bytes written per copy per pass; an instance written once
  a frame and read by every pass of that view is 64.
- Foliage in captures and point shadows: those passes spend the instance index on faces, and a
  draw per copy there brings back what instancing removed; the hill does not need it yet.

## Replaces
nothing. Carries out 0375 point 3 for 090.
