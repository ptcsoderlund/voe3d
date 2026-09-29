# 0295 — The player's hull is given a collider when it has none
date: 2026-09-29
by: planner

## Decision
For 041, bug 01: the lives system gives the first hull, each step it has no collider, a solid
box collider of size (4.64, 4, 2.62), the enemy tank's (card 06), through the structural queue.
It is added in the running game only, so it is never saved into the sponsor's
`tank_body.prefab`. A collider the sponsor puts on the hull is kept and wins.

## Reasoning
Enemy shells flew through the player because nothing on the hull is solid: 0294 point 6 left its
collider to the sponsor, and `tank_body.prefab` has none, so a sweep has nothing to hit and no
life is lost. Agents do not edit that prefab (0251, 0272), so the game cannot rely on a human
step for a rule it depends on. The lives system already owns "which hull is the player's" and
adds a row to it at run time, so the collider is one more row from the same place. Rejected: a
human-only fix (the bug comes back with any fresh prefab); a collider on every hull (only the
first is the player's).

## Replaces
Amends 0294 point 6: the sponsor may still put a collider on the hull; the game no longer needs
one there.
