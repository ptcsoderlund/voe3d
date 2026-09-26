# 032 — A sound when I take a coin

## What
Taking a coin in the coin game plays a short sound.

- Only the coin pickup. No music, no sounds for the menu, You win or Game over. Those come later.
- The sound is a file in the project. Which one is set in the Inspector, so I can swap it for
  another without touching code. The planner decides which component carries it and which file
  formats are read; one common format is enough.
- Taking two coins quickly plays the sound twice; the second does not cut off or wait for the
  first.
- The sound plays in Play and in the game Ship makes (031), and the shipped folder carries the
  sound file with it.
- A missing or broken sound file is not a crash: the game runs silent and says why on stderr.
- A starter pickup sound ships in `examples/coin_game/` so it works out of the box.

## Why
Taking a coin should feel like something happened. It is the last missing piece of 0186's fifth
step, and good enough is good enough: one sound, done.

## How to test
1. Open `examples/coin_game/` in the editor, press Play, Start, and run into a coin: I hear the
   pickup sound.
2. Run through two coins close together: I hear the sound twice.
3. On the menu, You win and Game over, there is no sound.
4. Stop. In the Inspector, point the coin's sound at another file, Play: I hear the new sound.
   Save, close and reopen: it is still the new one.
5. Point it at a file that does not exist, Play: the game runs, silent, and stderr says which
   file it could not read.
6. Ship, copy the shipped folder away from the engine as in 031, run it: I hear the pickup sound.
