## Learned
- The tree pick (`voe_3d_tree_pick`) and the grown lattice (`3d/src/foliage_grow.c`) were keyed by a kind's index, so removing a kind would have re-picked every later kind's variations and moved its grown copies; 0517 keys them by a per-kind seed.
- The game tree cook writes tree values as designated initializers, so a `.tree` key it omits cooks as 0 in Play and Ship, not as its default.
- The landscape cook (`editor/src/game_tree_landscapes.c`) writes each kind's look as a positional initializer, so a look field it omits cooks as 0.
- The `--capture` abort on an open `.tree` is avoided on the 117 branch by commit 1ec4325b; a mental-model line saying it aborts is stale.
- A foliage copy's kept-LOD key (`3d/src/lods_kept.c`) mixes the kind index; after a removal it settles in one frame, so it was left on the index.

## Assumed
- A kind is removed by a × on its title row in the Foliage section, at once, with no confirm.
- A kind brought back by undo comes back unfolded; the other kinds' folds follow the removal and its undo.
- The foliage brush's picked kinds follow a removal and its undo, the removed kind unpicked.
- A look edit never changes a kind's seed; the 3d store keeps the held one.
