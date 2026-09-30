# 0296 — Only the player's shots wreck
date: 2026-09-29
by: planner

## Decision
For 041, bug 02: a shell's hit swaps a `tank_breakable` target for its wreck only when the shot
is the player's: it has a `tank_shot` row and the `tank_lives` row's entity is its `owner` or
under it (`voe_scene_parent_within`). Any other hit (an enemy's shot, a shot with no row, no
lives row yet) stops the shell and changes nothing, as a hit on a wreck does. This holds for
houses as for enemy tanks.

## Reasoning
Enemies fire toward the player (0294 point 5) and their shells hit other enemies on the way;
0294 point 3 swapped any breakable hit, so enemies wrecked each other while the player held fire.
The shell system already owns the swap, so the rule is one test there. The lives row is what
already names the player's tank (0295). Rejected: "the owner has no `tank_enemy` row", which
breaks when an enemy is removed while its shell flies; enemy shells ignoring enemies in the
sweep, which would let them fly through tanks.

## Replaces
Amends 0294 point 3: only the player's shots swap.
