# 0262 — The coin game's camera arm glides both ways and snaps in only when blocked
date: 2026-09-26
by: planner

## Decision
For 030 (bug 01), in `examples/coin_game/Code` only. Each step the spring arm tests the
length `probe = max(arm, distance)`: clear is the whole probe, or else the longest clear length
bisected over [0, probe] to 1 cm, with 0261 point 5's capsule. Then:

1. **Snap in only when blocked where the camera is:** clear shorter than the arm sets the arm
   to clear at once.
2. **Glide otherwise, both ways:** the arm moves towards min(distance, clear) by at most
   10 m/s times the step's seconds, nearer or farther alike.

## Reasoning
The wheel's shorter distance is not a wall, so it glides like a longer one. Testing up to the
arm's current length, not only the chosen distance, keeps the camera from sitting inside a wall
while it glides in from beyond the new distance. It needs no more overlaps than before.

## Replaces
0261 point 5's last sentence ("comes in at once and goes out at most 10 m/s").
