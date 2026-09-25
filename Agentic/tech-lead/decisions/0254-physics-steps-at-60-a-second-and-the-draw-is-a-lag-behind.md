# 0254 — Physics steps at 60 a second, and the draw is a lag behind the last step
date: 2026-09-25
by: planner

## Decision
ADR-0065's D-082 and D-083, for 027:
1. **The fixed step is 1/60 s, at most 4 steps a frame.** Time past the fourth step is dropped,
   so a machine under 15 frames a second runs in slow motion rather than falling behind. The
   accumulator is the game's run (`game`); the editor steps nothing.
2. **One step, in order**: the transforms remembered, the project's systems at 1/60 s, the world
   step (structural queue, replaces, every owning system), the bodies' move, the transform
   system again so the next step's queries see where the bodies went.
3. **The previous state is a runtime-only table in `scene`**, owned by the transform system and
   registered only by worlds that step (the game's world). `remember` copies every transform
   into it at the start of a step; `between(entity, lag)` gives the transform `lag` of a step
   back from the current one: position in double, rotation the shortest normalised blend, scale
   straight. No previous row, or a lag of 0, is the current transform.
4. **The draw takes `lag`** = 1 − banked time / step. The editor passes 0 and draws what is.

## Reasoning
60 a second matches the common display and keeps a capsule's substeps few; four steps covers
every frame rate a person would play at. A lag rather than an alpha makes nought mean "now", so
every caller that does not step (editor, dev, tests) passes nothing special. Keeping the table
runtime-only means it is never saved, never in the Inspector, and costs nothing in a world that
does not register it.

## Replaces
nothing. Answers D-082 and D-083 of ADR-0065.
