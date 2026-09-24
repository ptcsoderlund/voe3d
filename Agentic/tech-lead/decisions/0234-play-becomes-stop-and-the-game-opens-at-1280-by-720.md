# 0234 — Play becomes Stop while the game runs, and the game opens at 1280×720
date: 2026-09-24
by: tech-lead

## Decision
While the game started by Play (0187) runs, the editor's Play button reads **Stop**, and pressing it
closes the game. Closing the game's window stops it too. The game has no key of its own to quit:
Escape does nothing special, and a developer with a fullscreen game uses Stop or the system's
close key (Alt+F4). The game opens in a **1280×720 window**, whatever the editor's size or layout.
A per-project setting for window size comes later. Showing the compiler's errors in the editor
(0187) moves to milestone 3, the first time a developer's own code can fail to build. Until then
a failed build starts nothing and puts nothing on screen.

## Reasoning
You stop the game about as often as you start it, so Stop sits where Play is. A fullscreen game
has no window to close, and Stop covers that. Escape is left to the game, since many games use it
for their own menus. A fixed size keeps the game independent of how the editor is laid out.
Errors wait because nothing in milestone 2 can cause one. Alternatives rejected: close the window
only (no way out of a fullscreen game); Escape quits (takes a key from every game); a game window
the size of the scene view (ties the game to the editor's layout).

## Replaces
nothing
