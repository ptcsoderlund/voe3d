# 18 — Play and Ship grow the forest at start
folder: game
after: 17, 06
decisions: 0168, 0515, 0513, 0370

## Change
`game/src/run.c`: after each models update and before the materials,
`voe_game_trees_grow(models, device, &voe_game_trees_cooked, scratch, progress)`. At start this runs under
the splash with its progress, so variations are grown before the first frame.

Update `run.c`'s header and its `game/src/src.md` entry: the cooked names now include the trees.

## Done when
`cmake --build --preset debug --target voe_game` exits 0.

The human, in a project with a spruce and a birch species, works through `feature.md`'s How to test 1–12:
1. create and open, and a tree stands;
2. each slider changes the tree with no hitch, and joins look clean;
3. Variations 6 gives six kin, and 1 gives one;
4. a birch reads as a different tree;
5. both are painted as kinds, with no visible repeats;
6. after a reopen, the same variation stands in the same spot;
7. changing droop changes only the spruces;
8. a ground material grows the birch;
9. trees simplify without popping, foliage stays under 3 ms (`VOE_GAME_COUNTS=1` or the GPU ms), and they
   cast shadows;
10. a Blender kind still works;
11. `Assets/` holds only small `.tree` files, a hand edit is taken in, and `Docs/` describes the file;
12. Play and Ship look the same.
