# 06 — It should say LOD, and go to six

## Seen
"I want it to say "Make LODs" instead of "Levels". It should also say "LOD" in inspector. I would also like 6
level of LOD's."

## Expected
Decision 0433. The Assets menu says **Make LODs**, the question says "LODs of <file>" with a **LODs** row of 1
to 6, and the Inspector shows **LOD** where it showed Level. A model whose LODs I never chose gets 6, or fewer
when it is too small to cut that far.

## How to reproduce
1. Right-click a `.glb` in Assets: the row says Make levels, and the question offers Levels 1 to 4.
2. Select a placed model: the Inspector says Level.

## How to test
1. Right-click a `.glb` in Assets: the row says **Make LODs**. Press it: the question is "LODs of <file>" and
   the LODs row offers 1 to 6, with 6 lit for a model I never set.
2. Choose 6 and Medium on the stable, Make. Select a placed stable and fly slowly away: the Inspector says LOD
   0, 1, … up to 5, and the Frame panel's Triangles keeps dropping at each.
3. Make LODs on a small rock with 6: it stops at fewer LODs, and the Inspector says how many it has.
4. Choose 2 on a model: the question shows 2 when opened again, and the Inspector goes no higher than LOD 1.
