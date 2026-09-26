# 0260 — The coin game keeps its state on the player and hides a taken coin
date: 2026-09-26
by: planner

## Decision
For 029, how `examples/coin_game/Code` holds its rules:

1. **The numbers.** Player carries `speed`, `jump_height`, `gravity`, `camera_distance`
   (default 6), `start_score` (INT32, default 1000) and `score_drop` (INT32 per second, default
   10). Coin carries `points` (INT32, default 100), so a coin is not an empty struct. Rotator
   carries `degrees_per_second` (FLOAT32, default 90).
2. **The run's state is a runtime-only row**, `game_state`, added to the first player entity and
   written only by the game module (its step system and its interface): the phase (menu,
   playing, won, lost), the score dropped so far, the part-second banked, a restart count, and
   Enter's and Escape's last levels. The score is `start_score − dropped + points of coins taken
   since the last restart`, never below 0.
3. **A taken coin is hidden, not destroyed**: its Shape is removed and a runtime-only
   `coin_taken` row keeps that shape and the restart count it was taken under. Only takes under
   the current count are taken; an older one is restored by the coin system (Shape added back,
   row removed). Restart bumps the count; the player system then moves the player to the start it
   recorded in its runtime-only `player_state` row and stops it.
4. **Play stops off the playing phase**: no walk, no jump, no tick, no take; gravity still
   applies so the capsule rests; rotators always spin.
5. **The camera** is the scene's one camera (0218), kept `camera_distance` behind the player
   along its own forward, after the move.

## Reasoning
Runtime-only rows keep the Inspector and the scene file to what the sponsor sets. Counting takes
by restart makes Restart one number written by one owner, with no step in which stale takes are
read. Rejected: rebuilding the world from the cooked scene (an engine change that frees GPU rows
for one game); destroying coins (nothing to put back); scale 0 (a degenerate matrix drawn and
shadowed); a separate game-numbers component (the sponsor's level lists only Player, Coin and
Rotator).

## Replaces
nothing.
