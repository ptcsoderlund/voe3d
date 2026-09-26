# 0261 — The coin game's camera turns by a held right button and springs in by overlap
date: 2026-09-26
by: planner

## Decision
For 030, in `examples/coin_game/Code` only:

1. **The numbers stay on Player** (as 0260 point 1): `camera_distance` becomes the start
   distance (default 6); new `camera_turn_speed` (degrees per motion unit, default 0.25),
   `camera_distance_min` (default 2) and `camera_distance_max` (default 12).
2. **A runtime-only `player_camera_state` row on the camera entity**, written only by a new
   `player_camera` module: yaw, pitch, chosen distance, the arm's current length and whether
   the lock was last asked for. Its first step fills yaw and pitch from the camera's authored
   rotation (forward f: yaw = atan2(−f.x, −f.z), pitch = asin(f.y)), clamped.
3. **Input is read once a frame**, in the interface entry point before the screens, because
   motion and the wheel drain each poll and a step runs zero to four times a frame. Only in
   the playing phase with the right button held: the pointer is locked (asked on a change
   only) and motion turns yaw and pitch, mouse up looking up; off it the lock is let go and
   nothing turns. The wheel works in the playing phase: each notch towards the person
   multiplies the distance by 1.15, away divides; clamped to [min, max].
4. **Pitch is held in [−80°, −5°]**, yaw wraps; the rotation is yaw about +Y then pitch
   about +X, so it never rolls or flips. The camera sits at the first player's position plus
   the rotation's +Z times the arm, after the move (0256).
5. **The spring arm is bisected capsule overlaps**, since `physics` has only overlap (0253)
   and 030 changes nothing outside the coin game: a capsule of radius 0.25 m from the player
   to the wanted end, ignoring the player; a non-trigger contact means blocked, then the
   length is bisected to 1 cm (overlap grows with length). The arm comes in at once and goes
   out at most 10 m/s.
6. **The walk follows the yaw**: W is the camera's forward flattened to the ground, S back,
   A and D its left and right; with no camera row yet, W is −Z as before.

## Reasoning
One component stays the whole tuning panel. A per-frame read loses no motion and counts none
twice. Overlap bisection costs about fifteen queries a step, cheap at this level's size, and
needs no engine change; a ray or sweep in `physics` replaces it when the engine gains one.
Rejected: reading the mouse in a step (lost or doubled motion); a separate camera component
(the sponsor's level lists only Player, Coin and Rotator); marching fixed spheres (hundreds
of queries at a large distance).

## Replaces
0260 point 5.
