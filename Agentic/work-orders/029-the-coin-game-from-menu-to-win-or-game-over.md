# 029 — The coin game, from menu to win or game over

## What
The coin game of 0186 exists as its own project, `examples/coin_game/` (0251). Agents write all
of its code; I build its level in the editor.

The project brings its own components, which I add in the editor like any other:
- **Player**: the capsule I control. It runs and jumps, and a third-person camera follows it, as
  in `examples/capsule/`. Where it stands in the scene is where it starts.
- **Coin**: marks an entity as a coin. Touching it takes it.
- **Rotator**: one field, degrees per second around Y. It spins any entity it is on, coins or
  anything else, but only in the game and never while I edit.
- The game's numbers, which I set in the Inspector: the starting score (default 1000), how much
  it drops per second, and how many points a coin adds (default 100). The planner decides which
  component carries them.

When the game starts, a **main menu** shows over the level with **Start** and **Quit**. Start
begins play. The **score ticks down** every second, and each coin taken adds its points. A
**HUD** shows the score and the coins left.
- The score reaches 0: **Game over**, with **Restart** and **Quit**.
- I take the last coin: **You win**, with the final score, **Restart** and **Quit**.

Play stops on both screens. Restart puts every coin back, the capsule at its start and the score
at its starting value, and play begins again. Menus and end screens are panels drawn over the
level, not separate scenes. Their buttons work with the mouse, and also with the keys: Enter
presses the first button and Escape the last.

No sound yet. Sound is the next work order.

## Why
Milestone 5 of 0186: the first time the engine makes a game you can win and lose, not a showcase.

## How to test
1. Open the editor and open `examples/coin_game/`. Add component offers Player, Coin and Rotator
   next to the engine's own.
2. Build a small level: a floor with a collider, a capsule with Player and a body, a camera, the
   sun, and five coins (squashed cylinders with a trigger collider, Coin and Rotator). Save it.
3. The coins do not spin in the editor.
4. Press Play. The game window shows the level behind a main menu with Start and Quit. The coins
   spin. Nothing moves the capsule yet, and the score does not tick.
5. Click Start. The menu goes. The HUD shows 1000 and 5 coins left, and the score drops each
   second.
6. Run and jump to a coin. It disappears, the score jumps up by 100, and coins left goes to 4.
7. Take all five. **You win** appears with the final score. The capsule and score stop.
8. Click Restart. All five coins are back, the capsule is at its start, the score is 1000 and
   dropping.
9. Stand still until the score reaches 0. **Game over** appears. Press Enter: the level restarts
   as in step 8.
10. Reach Game over again and press Escape: the game quits. Play again, then click Quit on the
    main menu: the game quits.
11. Stop. In the editor, set the starting score to 50 and a coin's Rotator to a much higher
    speed, then Play. Game over comes within a minute, and that coin spins faster.
12. Save, close and reopen the editor and the project. Every Player, Coin, Rotator and game
    number is as I left it.
