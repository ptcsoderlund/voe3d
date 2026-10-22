# 0464 — A view skips by world spheres in 3d, and foliage copies by tier
date: 2026-10-22
by: planner

## Decision
For 091, carrying out 0395.

1. **3d culls, on the CPU, before the draw.** Each drawable's world sphere about the frame's eye at
   the frame's lag is tested against a volume of planes taken from a view's projection × view. Render
   is unchanged: it cannot know a landscape node's heights or a water's waves, and 3d knows every
   drawable's true size. The planes live in one internal file, `3d/src/cull.h`.
2. **The camera pass** tests its six planes. A mesh row's sphere is its geometry box's
   (`voe_render_geometry_box`), a model row's the entry's `centre` and `radius`, a landscape node's its
   box over the heights, a particle's its quad's, a water's its quad's grown by its tallest wave.
   Panels, marks, the outline and the gizmo are never culled and never counted.
3. **Each cascade** tests its sides and its far plane and leaves its sun side open, so a caster
   further toward the sun than the box still draws as it does today. The point-shadow pass and the
   bounce are unchanged: render already culls the first by sphere, and the bounce sees the level.
4. **Foliage is gathered once a view in three tiers**: seen (in the camera's volume), cast (not
   seen, inside a cascade of a casting sun of the frame), unseen. A group's key gains its tier.
   The camera draws seen groups, the cascades seen and cast, the bounce sun maps all three, so the
   bounce's picture is unchanged. The gather fits the suns' cascades itself with
   `voe_3d_shadow_cascades_fit`, which has no state, so it agrees with the shadows call; it runs
   after `voe_3d_draw_system_lights` so it knows every sun. `VOE_3D_FOLIAGE_GROUPS` triples to 768.
5. **Counts.** A thing is a mesh row, a model row, a landscape node, a foliage copy (a band's `rest`
   copies not counted), a particle or a water. `voe_3d_view_counts` holds the camera pass's `drawn`
   and `skipped` and the cascades' `cast` and `cast_skipped`, summed over every cascade of every
   light. `voe_3d_frame.counts` carries the gather's and the shadows' part; `_run` returns the whole.

## Reasoning
Culling inside render's draw would cover every caller in one folder, but a landscape node's and a
water's geometry is displaced in the vertex stage, so its sphere there is wrong and nodes would pop.
A sphere tested against planes is conservative: nothing visible is ever skipped, so nothing pops at
the edges (How to test 3). Leaving a cascade's sun side open keeps every shadow cast today.
- Foliage culled per pass: an instanced group cannot skip copies, and gathering per cascade would
  write five times the instances. Tiers write each copy once, as now.
- Cast tier by a sweep along the sun: looser than the cascades and a second rule to keep in step.

## Replaces
Nothing. Carries out 0395; amends 0442 point 5 (a group's key gains its tier).
