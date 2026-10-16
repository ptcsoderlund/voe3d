# 0436 — LOD shares and swaps are per-level keys, and the levels file carries five swaps
date: 2026-10-16
by: planner

## Decision
For 089 bug 08, the choices 0435 left to the planner.

1. **The struct.** `voe_assets_levels` keeps `count` and `strength` and gains `float shares[6]` and
   `float swaps[6]`, indexed by level as the code counts it: index 0 is LOD 1, always 100 and ×1 and never
   read or written. Defaults 100, 50, 25, 12, 6, 3 and ×1 each. The `voe_assets_levels_swap` enum goes.
2. **The keys.** `[Levels]` gains `share1`…`share5` and `swap1`…`swap5`, numbered from zero as `.parts`,
   levels files and `_LOD<n>` stay (0434): `share1` is LOD 2's. Numbers are read with `strtof` and written
   `%g`. Shares lie in (0, 100) and each is strictly lower than the one before, after every key is read;
   swaps lie in ×0.1 to ×10 (`VOE_ASSETS_LEVELS_SWAP_LEAST`, `_MOST`). Anything else is MALFORMED.
3. **Old `.parts` files.** The old word `swap="near|normal|far|farther"` still reads, as ×0.5, 1, 2 or 4
   for every `swap<N>` its own key does not give, in any key order. It is never written again: a save
   writes all ten keys.
4. **The levels file, version 5.** The head's one swap becomes five floats, levels 1 to 5, at a fixed
   offset, so a swap-only change patches a fresh file in place
   (`voe_3d_model_levels_swaps_set`) and needs no remake. A file whose own levels were measured has
   `own` set, its level count and one error per level, and no parts. Version 4 files are stale and made again.
5. **The cut.** Each part aims at its own triangles × the level's share. A level that is not below the
   level before it in triangles is dropped with the ones after (the three-quarters rule goes). Strength
   keeps its angle and collapse limit and loses its shift.
6. **Own levels measured.** Each own level's error is the largest distance, both ways, between its bake
   and level 0's bake, vertex to nearest triangle, found through a uniform grid of triangle boxes.
   The store bounds own levels by error and multiplier as made ones, kept non-increasing; the 2^-k rule
   is only the fallback when no fresh file is there.
7. **Where the swaps reach the store.** The game's loader, finding a fresh file and a `.parts` that reads
   with other swaps, patches and rewrites the file before handing it on. A shipped game has no `.parts`
   and keeps the file's swaps.
8. **The panel.** "LODs of <file>" keeps its LODs and Strength rows (hidden for own levels) and drops the
   Swap row. Each LOD from LOD 2 on has a row: its name, a share number box in % (tenths, kept below
   the share before it, later ones pushed down to stay in order), a swap number box in × (tenths,
   0.1 to 10) and the triangles the LOD really has, "—" when not known. Own levels show their share
   as a label. Make with only the swaps changed on a model whose levels file is fresh saves and reloads
   the model at once, with no splash; any other Make prepares with remake as before.

## Reasoning
Keys per level read like the `material<N>` keys beside them and need no list syntax the sectioned reader
lacks. Arrays indexed by level spare every reader an off-by-one. Five fixed swaps in the head make the
swap-only change a patch of twenty bytes, which is what lets it skip "Preparing". A grid keeps the
measure linear on models of a hundred thousand triangles. Patching in the loader, not the editor, puts
Play, Ship and the editor on one path.

## Replaces
Carries out 0435. Replaces 0430 point 2's per-level cut and 0432 point 1's four swap words, which stay
readable only as 3 says.
