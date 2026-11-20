# 0514 — Far trees thin their leaf cards, and 117 draws no group pictures
date: 2026-11-20
by: planner

## Decision
In 117 the tree maker makes no group pictures. 0513 allows them where they are faster; here it uses none.
Each grown variation has four levels of its own, nodes `tree_LOD0`–`tree_LOD3`. Each level's error is
measured both ways against level 0, as an imported model's own levels are, so 0495's swaps and 0500's
dissolve take them unchanged.
- Level 0: trunk, branches and twigs as tubes, and every leaf card.
- Level k ≥ 1: twigs are no longer tubes, though their leaves stay. Tube sides and lengthwise segments
  halve at each level, down to 3 sides and 1 segment. Every 2^k-th leaf card is kept, and each kept card
  is scaled by √(2^k) about its centre, so the crown covers about the same area within the same outline.
- A leaf card is one quad, not crossed. It is rolled at random about its twig, and its normals are bent
  away from the crown's centre so the crown shades as a mass.

## Reasoning
Cut-out foliage costs by the pixels it covers (0508, 0513). A group card also covers the gaps between
leaves, so it saves triangles but not pixels. Thinning and enlarging saves the same triangles, keeps the
covered pixels about level, and needs neither a renderer inside the grower nor a picture rebuilt on every
load. Measured errors keep the swaps calm.
If the hill's forest misses 114's 3 ms while bound by triangles or vertices rather than pixels, the next
step is a group-card level, decided again then.
- Crossed quads: twice the overdraw for a fuller look from the side.
- A crown impostor at the far level: a render pass in the grower, a picture per variation on load, and
  more memory. Reconsider it if thinning measures short.

## Replaces
Nothing. Settles 0513's grouping rule for 117.
