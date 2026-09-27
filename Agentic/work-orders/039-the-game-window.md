# 039 — The game window

## What
A project has settings in the editor: the game window's size, and whether it opens windowed or
fullscreen. Play and the shipped game both use them. The game window can be resized or made
fullscreen at any shape, and the picture fills it without stretching. A wider window sees more at
the sides. For the tank game, the level's width always fits, and a wider window shows more water.
This ends 0234's fixed 1280×720.

## Why
Milestone 6 of 0268. The tank game should fill any screen and look right at any shape.

## How to test
1. In `examples/tank_game`, open the project settings. Set the size to 1600×900, windowed. Play. The
   game opens at that size.
2. Drag the game window's corner to a tall narrow shape, then a very wide one. Nothing stretches. The
   level's width stays in view, and the wide window shows more water at the sides.
3. Set fullscreen and Play. The game fills the screen. Stop from the editor, and it closes.
4. Ship, and run the shipped game. It opens fullscreen too.
5. Close and reopen the editor. The settings are kept.
