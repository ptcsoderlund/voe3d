# 23 — A leaf card takes the species' width and roll
folder: 3d
after: 22
decisions: 0168, 0516, 0517, 0514, 0515

## Change
First, in `3d/tests/tree_grow.c`, before any code changes: a test that pins today's default tree. Grow it
with `voe_3d_tree_grow` and `voe_3d_tree_mesh`, default values, variation 0. Pin the level-0 leaf
primitive's vertex count and the sum of its positions, to 1e-4 relative. The leaf primitive of level k is
`primitives[2k + 1]`, four vertices a card. It must still pass after the change at roll 0, width 1.

`3d/src/tree_mesh.c` (`card`, `place_cards`, `put_card`, `leaves`):
- **Width.** A card is `leaf_size` along its twig and `leaf_width` × `leaf_size` out from it, hinged on the
  twig as today.
- **Roll.** Keep today's random roll, drawn as today. Turn it toward the nearer of the two flat rolls (the
  card's out edge in the twig's level plane) by `leaf_roll` × fade. The fade is min(1, sine of the twig's
  angle from vertical / sin 15°), 0 at vertical. No draw is added, removed or reordered.
- **Far levels.** Both edges of a kept card scale by the same √(2^k), so its ratio holds.

`3d/include/3d/tree_grow.h`: the header's card point says the card's length and width, the pull toward
flat, the fade near vertical, and that far levels keep the ratio. Fix the tree_mesh entry in `3d/3d.md` if
it names the card's shape.

New tests in `3d/tests/tree_grow.c`:
- at width 0.5 and 2, every level-0 card's out edge is that share of its twig edge, to 1e-4;
- at levels 1–3 the kept cards keep that ratio;
- at roll 1 and width 0.5, every card whose twig edge is more than 30° from vertical has a level out
  edge: its y is at most 1e-3 of its length;
- at roll 1, both sides of a twig carry cards: some out edges point one way across the twig, some the
  other.

## Meaning
- Believed (precog): `leaf_size` is the card's length along the twig. Today's square cards do not show
  which `card` axis lies on the twig. The coder confirms it in `place_cards` or blocks.

## Done when
`ctest --test-dir build/debug -R '^3d/tree_grow$'` exits 0 with the pinned test and the new tests in it.
