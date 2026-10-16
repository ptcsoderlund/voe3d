# 0430 — A model's levels cut through soft edges and along seam lines, and its parts file says how many and how hard
date: 2026-10-15
by: planner

## Decision
For 089 bug 03.

1. **Seam lines move.** A position whose vertices fall into exactly two sides, with exactly two seam edges
   through it and no border, moves along one of those seam edges onto the position at its other end; each
   side's corners take the vertex the same side's triangle on that edge names there, so both sides of the
   seam keep their own uv, normal and colour. A side is the vertices at a position with equal uv and
   colour complement and normals within the strength's angle. Any other position of two or more sides
   never moves, as before. Always on.
2. **Strength**, per model, one of three:
   - **Gentle**: normals must be equal to share a side (0°); each level aims at half the one before; a
     collapse may cost up to a quarter of the radius. 0425 as it was, plus point 1.
   - **Medium**, the default: normals within 60° share a side, so a faceted log's hard edges soften;
     half per level; a quarter of the radius.
   - **Strong**: any normals share a side (180°); each level aims at a quarter of the one before; up to
     half the radius.
   A collapsed soft position's corners take the kept vertex's normal. The level's error, its bound on
   screen (0425 point 3) and the three-quarters rule are unchanged.
3. **Count**: 1 to 4 levels in all, level 0 included, default 4. The list may still end sooner by the
   three-quarters rule; 1 makes no level.
4. **Kept in the model's `.parts`** (0420) as an optional `[Levels]` section, `count=<1–4>` and
   `strength="gentle"|"medium"|"strong"`, each absent read as its default. Import writes the `.parts`
   keeping the levels of the one there, else the defaults. The cook carries them in the parts table.
5. **The loader reads them**: when `game` makes a levels file (0426 point 2) it reads the `.parts` beside
   the `.glb` (its `.glb` ending made `.parts`) for `[Levels]`; none, or one that will not read, gives
   the defaults. A shipped game has no `.parts` and its levels files are fresh, so it makes none. This is
   the one read of a `.parts` outside the editor, and only for levels (amends 0420 point 1, 0236).
6. **Asked in the editor.** After an Import of a `.glb`, and on Make levels (0429), a panel "Levels of
   <file>" shows Levels 1 2 3 4 and Gentle, Medium, Strong, the model's values lit, with Make and Cancel.
   Make writes the `.parts` and runs the prepare with remake (0429 point 3) under the splash. Cancel makes
   nothing; the loader makes the file at the first load.
7. **Levels files go to version 2**, so every file the earlier simplifier made is stale and made again at
   load with the model's values. The file does not record the values: a `.parts` changed by hand takes
   effect at the next Make levels or change of the `.glb`.

## Reasoning
The stable that stopped at two levels is one mesh of 47,452 triangles on 25,581 positions. 18,026 of them
were seams — 17,496 because their normals differ (flat-shaded logs, 30° between facets), 12,400 by uv —
and 0425 point 2 moved none of them. 6,755 free positions can lose about 13,500 triangles: level 1 stopped
at 33,202 (70 %), level 2 could not reach three quarters of that, and the list ended. A count changes
nothing there. Softening hard edges under 60° and moving uv seams along their lines leaves 23,351
positions movable, enough for an eighth. The angle and the cut per level are the strength a person asked
for; the error still decides when a level is drawn, so a stronger level only swaps in later. Kept in
`.parts` because it is the model's authored file, renamed and trashed with the `.glb` already, while
`Cache/` may be deleted.

## Replaces
Amends 0425 point 1 (count, cut and limit per model) and point 2 (seam lines, the strength's angle),
0426 point 1 (version 2), 0420 point 1 (`[Levels]`, read by the loader).
