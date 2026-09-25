# 0252 — The sun casts real shadows, in the engine, before the coin game
date: 2026-09-25
by: tech-lead

## Decision
The scene's one light casts **real shadows at run time**: every shape casts onto every shape,
in the editor's views and in the game, with no bake step. They are the landing cue the coin game
needs when the player jumps, so they come on 0186's road after collision (027) and before the
game's own work (0251), as milestone 5's first engine item. Shadows reach across an open-world
distance (0250), crisp near the camera and coarser far away, and do not shimmer or crawl when the
camera moves. A scene with no light draws unlit and without shadows, as before (0238). The
technique is the planner's; it must be the first step of the dynamic-lighting idea, not a
dead end beside it.

## Reasoning
Real shadows serve every later game and lead toward dynamic lighting, where a blob shadow is
coin-game code only. Rejected: a blob shadow under the player alone (crude, one game only); both
at once (the sponsor picked the long-term path; a game may still add a blob in its own code).

## Replaces
nothing. It adds shadows to milestone 5 of 0186.
