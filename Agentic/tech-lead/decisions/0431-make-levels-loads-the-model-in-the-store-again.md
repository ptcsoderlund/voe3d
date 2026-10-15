# 0431 — Make levels loads the model in the store again
date: 2026-10-15
by: planner

## Decision
For 089 bug 04.

1. Once the splash wait of a prepare (Import's or Make levels', 0426 point 3, 0429 point 2) has run,
   the editor loads that model again into its store from its `.glb` and the levels file just written,
   whatever the `.glb`'s stamp, so the new levels are drawn on the next frame. A path the store does
   not hold is left alone: it loads with the new file when first placed.
2. `game`'s loader gains `voe_game_models_reload` for it: the one model's read and levelled load the
   watch already does, without the stamp test. A failure keeps the entry as the store keeps any failed
   reload, and is said in the editor's notice as a broken file is.
3. The reloaded model wears its parts again through the update's materials call, as a watched one does.

## Reasoning
0429 point 4 held that the levels drawn are already the ones just written, because the making is the
same for the same bytes. 0430 made the count and strength part of the making, so Make levels with a
new strength writes different levels for the same bytes, and the store kept drawing the old ones until
the project was opened again. Watching the levels file's stamp too would put a second stamp on every
entry for a file only the editor's own prepare rewrites; reloading after the prepare is one call at the
one place that knows.

## Replaces
0429 point 4.
