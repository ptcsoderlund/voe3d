# 047 — The tank game

## What
`examples/tank_game/` is a whole game, shipped. By now it drives, shoots, hits and explodes (0272).
This adds the rest. The level scrolls forward and the tank cannot go back past the bottom of the
screen. Enemies come in waves the sponsor places in the level. There is a score and lives, a boss or
end point to win at, and a game over. There is a menu to start, pause and quit, used by pad or
keyboard. The level is the sponsor's, built in the editor. The game is shipped from the editor and
runs on Linux and Windows.

## Why
Milestone 14 of 0268. The game is what proves 0.2, and shipping it closes 0.2.

## How to test
1. Ship the tank game and run the shipped program. The menu shows. Start with the pad.
2. The level scrolls forward as you drive, and you cannot back out of the screen's bottom.
3. Waves of enemies come as the level goes on, where the sponsor placed them. The score rises as they
   die.
4. Lose every life. Game over shows, and the menu can start again.
5. Play to the end and win. A win screen shows.
6. Pause mid-game with the pad, and with Escape. The game stops, and it resumes where it was.
7. Do the same on Windows with the shipped game.
