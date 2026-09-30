# 0297 — An enemy aims its turret before it fires
date: 2026-09-29
by: planner

## Decision
For 041, bug 03: an enemy's turret is a `tank_turret` row on the enemy prefab's turret child,
turn 60 degrees a second, aim 180. The turret system turns only turrets within the entity of the
`tank_control` row, the player's hull; the enemy system turns its enemies' turrets toward the
player through one call the turret system exports, and so owns their aim. The `tank_turret`
table has `VOE_GAME_WORLD_MAX_DRAWN` rows, since spawned enemies carry one. An enemy fires only
when its barrel is within 2 degrees of the flat way to the player, from its `muzzle` in the
barrel's frame about the turret's world position, turned as the barrel is; an enemy with no
turret never fires. `from` stays the enemy's position (0294 point 1).

## Reasoning
0294 point 5 fired toward the player whichever way the tank faced, so shells left sideways and
backwards while the barrel stayed forward. Reusing the turret row and its turn keeps one turn
rule for both tanks and no new component. Rejected: a separate enemy turret component (a second
turn rule to keep in step); snapping the turret (the player cannot see it coming); firing along
the hull (a tank that cannot shoot beside it).

## Replaces
Amends 0294 point 5: the turret turns, and the shot leaves along it.
