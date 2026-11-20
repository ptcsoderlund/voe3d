# 0513 — A leaf is a textured card, and cards may gather leaves and twigs where that is faster
date: 2026-11-20
by: tech-lead

## Decision
A leaf grown by the tree maker is a textured plane, a card: a small quad (single or crossed) showing the leaf
material, cut out by its base-colour alpha and drawn double-sided (0404). "Leaf size" scales a card and "leaf
density" sets how many cards an outer branch carries. The tree maker may gather several leaves, and the thin
twigs that carry them, into one larger card showing a picture of that group. It may do this at any level,
near or far, and always does it where it is faster. The picture is rendered by the tree maker from the
species' own bark and leaf materials, and is rendered again when the project loads, as the mesh is, so
nothing but the species file is kept (0510). The rule for when to group, what to group, how big a group's
card is and how many levels use groups belongs to the planner. The test is measured speed: the hill's forest
stays within 114's 3 ms foliage budget, and a grouped card must make a tree cheaper at the distances where it
is used, without the tree visibly changing shape when a level swaps (0495, 0500). The same goes for the two
smaller points in 117: play and the shipped build grow the variations at start, under the splash, from the
species file, so the game library carries the grower. When a species' variation count changes, painted
trees may pick their variation again; each pick is a hash of the copy's (kind, x, z), taken modulo the count,
and stays the same across opens.

## Reasoning
The sponsor (2026-11-20): "yes, a leaf should be a textured plane. Maybe we can tune our system to batch a few
branches and leaves as one textured plane? Whichever gives us best performance." Cards are how tree tools
work (SpeedTree, Sapling), and the cost they bring is cut-out overdraw, which 114 and 0508 already pay down.
A card showing a group of leaves draws fewer triangles, but it is not cheaper by itself: a cut-out leaf costs
by the pixels it covers, and a group card covers its gaps too. So the choice is measured, not assumed, and
the measure is the one 114 set. Growing at start keeps one grower and fits 0510's "nothing but the species
file is kept".
- Solid leaf-shaped geometry: millions of triangles in a forest, and needles do not read.
- Single-leaf cards only: gives up the far levels' cheapest form.
- A picture of the whole crown at the far level only (an impostor): one case of the grouping above, which
  the planner may choose if it measures best.
- Grown meshes cooked into `Build/` for the game: a second path beside the grower, and more files kept.

## Replaces
Nothing. Settles the leaf left open in 0510, and the `[assumed]` line "Leaves are cards of the leaf material"
in `Agentic/cognition/tree-maker.md`.
