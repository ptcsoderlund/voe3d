# 03 — A large model gets only two levels, barely smaller

## Seen
"It says there are only two levels? Going from 250k triangles to 207k. That's not very good. This is a large
model, it's a stable, so it's meters in all directions.

I think I need to be able to choose how many levels and the intensity for them when importing.

Find out first why the levels stop at 207k: a stable is many separate pieces, with seams between parts,
materials and UV islands. If the simplifier won't go further, a count alone won't help. The intensity has to
let it go further."

Found: `wooden_stable.glb` is one mesh of 47,452 triangles on 25,581 positions; 18,026 are seams (17,496 by
flat-shaded normals 30° apart, 12,400 by uv), which the simplifier never moved. Level 1 stopped at 33,202
(70 %), and level 2 could not get under three quarters of that.

## Expected
Decision 0430. Seams move along their line; a strength (Gentle, Medium, Strong) lets normals within an angle
share a side and cuts harder per level; a count of 1 to 4 levels. Both are kept in the model's `.parts`, asked
after Import and on Make levels, and used whenever the levels file is made.

## How to reproduce
1. Open the lumberhill project and place `buildings/wooden_stable.glb`.
2. Select it: the Inspector says two levels; fly away and the Frame panel's Triangles drops only to about
   eight tenths.

## How to test
1. Make levels on the stable: the question shows Levels 4 and Medium; Make. Fly away: three or four levels,
   and Triangles far well under half of near.
2. Make levels with Levels 2 and Strong: two levels; the question shows 2 and Strong when opened again.
3. Import a `.glb`: the question shows after the copy.
