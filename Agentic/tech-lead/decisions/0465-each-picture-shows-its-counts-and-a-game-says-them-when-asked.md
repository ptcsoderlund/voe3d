# 0465 — Each picture shows its counts in the Frame panel, and a game says them when asked
date: 2026-10-22
by: planner

## Decision
For 091, where 0464's counts are read.

1. **The editor.** Each frame the editor keeps the counts of every picture it drew, in drawing
   order: the selected camera's preview, then each shown view. The Frame panel copies them with its
   quarter-second take and shows, under Draws, one row per picture, `View N` or `Camera`, with
   drawn and skipped, and one row under it for its shadows' drawn and skipped.
2. **A game, Play's and a shipped one alike,** has no Frame panel (0358 names the editor and dev).
   With the environment variable `VOE_GAME_COUNTS` set to `1`, read once at start through
   `voe_platform_environment_read`, it writes one stderr line a second:
   `game: view: drawn D, skipped S; shadows: drawn C, skipped K`, the last frame's counts. Unset,
   it writes nothing and reads nothing each frame.

## Reasoning
The counts are the CPU's own and exact the frame they are made, so they need no render lag; taking
them with the panel's refresh keeps them as readable as the times. A game run by Play shares the
editor's stderr (`editor/src/play.h`), so one variable serves Play and a shipped game alike, and a
player never sees it.
- A Frame panel in the game: a readout every shipped game would carry, past this feature.
- Counts per pass in render's timings: lost on a card without timestamps, and render does not cull.

## Replaces
Nothing.
