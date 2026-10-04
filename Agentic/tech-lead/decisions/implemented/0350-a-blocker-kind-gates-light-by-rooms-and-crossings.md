# 0350 — A blocker's kind gates light by Rooms and by crossings
date: 2026-10-04
by: planner

## Decision
How 0348 is carried out, for 056 bug 01:
1. **The row.** `voe_scene_light_blocker` gains `kind`, a UINT32 named Room 0, Indoors 1, Wall 2, so the
   default row, the unsaid row and a zeroed row are all Rooms and a 056 scene keeps its look. The drain
   refuses a kind past Wall.
2. **The pass's kinds are masks.** `voe_render_light_blockers` gains `walls` and `indoors`, bit i set when
   blocker i is that kind (neither is a Room), and `sun`, the mask of the blockers holding the directional
   light's world place. The 64-byte box record is unchanged. All three zero is 0347's picture. 3d works out
   `sun` with `voe_render_light_blockers_mask`, which becomes public in `render/device.h`.
3. **Direct light from a source s to a point p** (the surface pushed PUSH along its normal) passes when
   both: the Room bits of p's mask equal the Room bits of s's mask; and the segment from p to s — for the
   sun the ray from p against its direction — meets no Wall or Room blocker whose bit is not in s's mask.
   Indoors never stops direct light. A blocker is met by a sphere test on the segment, then slab clipping
   in its unit space, the face counted as met.
4. **Fill reaches p** when the Room bits of p equal the sun's and p is in no Indoors box. It is not
   stopped by a crossing, and its fade reads the sun's reach after rule 3, so a patch the sun is stopped
   from reads the whole fill.
5. **The bounce is direct light.** The read weights a probe by nought unless rule 3 passes with the probe
   as s and the surface as p. The relight lights a texel's hit by the sun and by a lamp by rule 3, and a
   texel feeds its probe only when rule 3 passes with the probe as s and the hit as p. A change of
   `walls`, `indoors` or `sun` relights.

## Reasoning
- A kind per record (a fifth float4, or the sphere's sign): every layout and stride moves; three words a
  pass carry it whole, and zero means the old behaviour.
- The render working out the sun's place: `voe_render_light` has no position, and 0349 will bring several
  directional lights each with their own; the caller already knows the entity.
- Indoors stopping direct light: the sun could not come through a window, which is what Indoors is for.
- Testing rule 3 only for Walls and keeping Rooms as a pure mask: a Room floating in the air would cast no
  patch, against 0348's "every blocker stops direct light".

## Replaces
Nothing. Carries out 0348, which amends 0347 points 3–4.
