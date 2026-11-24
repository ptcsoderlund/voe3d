# 0517 — A leaf lies flat by pulling its roll, and a kind keeps its draws by a seed
date: 2026-11-24
by: planner

## Decision
How 117 builds 0516.

1. **Keys.** `leaf_roll`, 0–1, default 0, and `leaf_width`, 0.1–4, default 1, in `[Tree]` after
   `leaf_density`. A card is hinged on its twig: the edge on the twig is its length, `leaf_size`; the edge
   out from the twig is its width, `leaf_width` × `leaf_size`.
2. **Roll.** Each card keeps today's random roll about its twig, drawn as today, and is turned from it toward
   flat by `leaf_roll` times a fade. Flat is the twig's level plane, on whichever of the two sides the random
   roll was nearer, so at roll 1 the sprigs lie on both sides of a branch. The fade is 1 for a twig more than
   15° from vertical and falls linearly to 0 at vertical, where there is no level direction: a near-vertical
   twig, such as a leader, keeps its random roll, and nothing jumps as a twig passes 15°. No draw is added or
   moved, so roll 0 grows exactly today's tree.
3. **Far levels** scale both of a kept card's edges by the same √(2^k), so its ratio holds.
4. **A kind's seed.** Every draw keyed by a kind (the grown lattice's hash, 0476 point 3; the tree pick,
   0515 point 4) takes the kind's seed, not its index. The seed is `seed<k>` in `[Foliage]`, 0–15, missing
   meaning k and written only when it is not k, so every file today reads and grows as before. Removing a
   kind keeps each later kind's seed as it moves down, so no tree changes its variation and no grown copy
   moves. A kind added takes its own index as its seed, or the least seed no kind has when that one is taken.
5. **Undo of a removal** is the existing landscape step (`editor/src/file_steps.h`): the whole landscape
   before and after, taken into the store, marked with the kind it removed so undo and redo move the folds
   and the brush's picked kinds with the kinds. A removed kind's fold is dropped, and undo brings the kind
   back open.
6. **The control** is a × at the right end of each kind's title row in the Foliage section. It removes at
   once; no confirm. The Delete key is not bound to it.

## Reasoning
- Pulling the drawn roll rather than drawing again keeps every other draw in place, which is what makes
  "a file without the keys grows as before" true by construction. Two flat sides, not one, because a spruce
  carries sprigs on both sides of a branch.
- The fade avoids a direction that does not exist, without a second rule (borrowing the parent's level) that
  would need a parent whose own direction can be vertical too.
- 0.1–4 lets a sprig be a narrow strip or a fan wider than long; past 4 a card is a ribbon nothing needs.
- Without seeds, removing a rock before the birches would re-pick every painted tree's variation and move
  every grown birch: step 15 of 117 would fail. Keying draws by a seed that defaults to the index changes
  nothing until a kind is removed.
- The landscape step already holds and takes whole landscapes; a stroke kind for removal would need a
  remove and an insert in the store for one rare action. The cost is two landscapes per removal held in the
  undo line, some 6 MB at 512 cells.

## Replaces
Nothing. Carries out 0516; amends the hash key of 0476 point 3 and 0515 point 4 (a kind's seed for its
index).
