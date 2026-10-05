# 01 — The Panels list has no background

## Seen
The Panels list has no background. Only the narrow tick column is dark; the panel names are drawn straight
over the scene, so they are hard to read where the scene is bright. The tick beside an open panel shows as an
empty box (a missing glyph), not a tick. Screenshot: `Skärmbild 2026-10-05 144942.png` in the repository root.

## Expected
The whole list, from the tick column to the end of the longest name, sits on one solid menu background, the
same colour as the top bar's, so every row reads clearly over any scene. An open panel shows a visible tick.

## How to reproduce
1. Open a project whose scene has a bright area under the top-left of the window (tank_game does).
2. Click Panels in the top bar.
3. Look at the list: the names have no background behind them, and the ticks are empty boxes.
