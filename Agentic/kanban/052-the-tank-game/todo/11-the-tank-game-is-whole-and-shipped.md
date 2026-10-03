# 11 — The tank game is described whole, and the human ships and plays it
folder: examples/tank_game
after: 03, 10
decisions: 0168, 0272, 0334
read: feature.md

## Change
Text only; agents never edit `main.scene` (0272). Read `examples/tank_game/tank_game.md` and
`examples/tank_game/Code/Code.md`.

- `examples/tank_game/tank_game.md`: the opening says the game is whole: it scrolls along −Z,
  enemies come in the sponsor's waves, a score and lives, a goal to win at, a menu, pause and
  game over by pad or keyboard. The `Code` entry adds the scroll, waves, score, goal, state and
  menu. The `main.scene` entry says the sponsor's level carries its spawners as waves and a
  Tank / Goal at its end.

## Done when
`grep -q Goal examples/tank_game/tank_game.md` exits 0, and
`git diff --quiet HEAD -- examples/tank_game/main.scene` exits 0.

The human's, in the editor on `examples/tank_game` (feature.md How to test):
0. Build the level in `main.scene`: ground running far along −Z, spawners placed along it as
   waves (Tank / Spawner, `count` in the Inspector), a Tank / Goal at its end.
1. Ship, then run the program in `Build/ship/<name>/`. The menu shows; Start with the pad's
   south button. The particle picture shows (card 03).
2. Drive forward: the level scrolls; back down: the tank stops at the screen's bottom.
3. Waves come as their spawners scroll into view; the score rises as enemies are wrecked.
4. Lose every life: Game over shows; Menu, then Start plays again from the beginning.
5. Drive to the goal: You win shows.
6. Pause mid-game with Start and with Escape: everything stops, sound too; resume goes on
   where it was.
7. Ship and play the same on Windows.
