# 0257 — The after-the-move slot is a third entry point, and its intents drain in the same step
date: 2026-09-26
by: planner

## Decision
For 027 bug 01, the shape of 0256's after-the-move slot:

1. A project defines a third entry point beside the two of `game/project.h`:
   `void voe_game_project_systems_after_move(const voe_game_project_step *step)`. Every project
   defines it; one with no after-the-move systems leaves it empty, and the no-code stub
   `cmake/game.cmake` writes defines it empty too. `voe_game_project_systems_run` keeps its name
   and is the before-the-move slot. Inside each, the project calls its systems in list order.
2. One step becomes: transforms remembered, `systems_run`, the world step, the bodies' move, the
   transform system, `systems_after_move`, then the world step again, so every intent an
   after-the-move system submits (a transform, a project replace, a structural change) is applied
   before the step ends, and is remembered and drawn at the same lag as the bodies.
3. `voe_game_steps_run` takes the two slots as two function pointers; `run.c` stays the one file
   naming the entry points (0245).

## Reasoning
A transform intent is applied only by a drain; without the second world step, a follow
submitted after the move would land in the next step, after `remember`, and the camera would
still trail by one step, which is the bug. A second world step drains everything with calls that
exist; a transform-only drain would leave other intents a step late. A required entry point over
a weak or looked-up one: the game links statically on Linux and Windows alike, and a missing
definition is a link error that names itself. Renaming `systems_run` to `_before_move` would
touch every project and the Windows export skip list for a name only.

## Replaces
nothing. Details 0256; amends 0254's step order.
