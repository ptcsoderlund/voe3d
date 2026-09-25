# 0243 — A project's code library builds and loads on Windows
date: 2026-09-25
by: tech-lead

## Decision
The editor builds and loads a project's code library on Windows exactly as on Linux: Refresh, opening
a project with code, Play refreshing first, the swap, the Errors panel and the Inspector's project
components all work in the Windows editor. `game.cmake` no longer refuses `VOE_GAME_LIBRARY` on
Windows. How the library's engine symbols bind to the editor's on Windows is the planner's. Linux
still alone verifies a card (ADR-0130); the Windows path is written by the agents and proved by the
human opening the example project on Windows.

## Reasoning
The human works on Windows too, and 025 is not usable there: opening the example stops at
"VOE_GAME_LIBRARY is Linux-only for now". Alternatives: keep Windows refused until a later feature
(leaves 025 untestable on a machine the human uses); run project code in the editor another way on
Windows only (two mechanisms for one feature).

## Replaces
Amends 0242 point 4: its "Windows: library mode is refused by `game.cmake` with a message, for now"
no longer holds.
