# 0241 — The editor loads the project's code, and a Refresh reloads it
date: 2026-09-24
by: tech-lead

## Decision
A project's own component types (0239) are **full editor citizens**: they show under Add component,
their fields are edited in the Inspector, and they are undone, saved and cooked like the engine's
own. To know them, **the editor builds the project's code as a library and loads it into its own
process**. A **Refresh** in the editor rebuilds and reloads it **without restarting the editor**,
and the open scene, unsaved changes included, comes back as it was. Play refreshes first. The
editor takes only the project's component types from the library. Project systems run only in the
game, never in the editor. If the build fails, the editor shows the compiler's output (0240) and
keeps the types it had. When a type the scene uses is gone after a refresh, the editor keeps that
data untouched in the scene and saves it back as it was, so restoring the type restores it. The
game still links the project statically (0187). Engine folders still link statically: the project
library is the one seam ADR-0008 foresaw for live reloading, not a plugin registry for the engine.
How the library is built, loaded and swapped is the planner's. The sponsor holds this open until it
has been tested.

## Reasoning
Restarting the editor after every change to a component type is too slow for everyday work
(sponsor), and a refresh that keeps the scene is what a developer expects. The cost is that the
project's code now runs inside the editor, so a crash in that code can take the editor down.
Rejected alternatives:
- rebuilding the editor with the project linked in and restarting it: annoying on every change.
- a helper program that prints the types as text: would be thrown away once the editor loads code.
- the project's code adding components when the game starts: cannot be edited in the editor.

## Replaces
In 0187, "nothing is loaded into a running process", for the editor only. The game is unchanged.
