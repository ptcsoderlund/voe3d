# 0294 — A tank shell sweeps, and a hit wrecks or costs a life
date: 2026-09-29
by: planner

## Decision
For 041, the tank game's half (0272):

1. **A shot carries a runtime-only `tank_shot` row**, added by the gun that fired it at the
   spawn: `owner` (the firing tank's root, ignored by its sweeps), `from` (where the next sweep
   starts: at the spawn the firing gun's or enemy's world position, so a muzzle past a wall
   still hits the wall), and `hit` with its `target`. The shell system writes it afterwards.
2. **The shell system gathers once a step and sweeps each shell** from `from` to where it
   would fly, with `tank_shell.radius` (default 0.1 m). On a hit the shell records it, does
   not move, and is removed that step; otherwise it moves and `from` becomes the new place.
3. **`Tank / Breakable`** has one field, `wreck`, a prefab name. A thing hit with it is
   swapped: the wreck spawned at its world place and it removed with its tree, once in a
   step however many shells hit it; a refused spawn leaves it. A wreck is not breakable, so a
   shell stops on it and nothing changes.
4. **Lives are a runtime-only `tank_lives` row** on the first hull, 3 at the start; each shot
   whose target is that hull or under it costs one, never below 0; the HUD shows the count.
   What 0 does is milestone 14's.
5. **Enemies fire**: `tank_enemy` gains `prefab`, `rate` (0.5 a second), `range` (30 m),
   `muzzle` and `wait`; within range of the lives row's hull an enemy fires along the level
   toward it. Its turret does not turn yet.
6. **Agents make the prefabs code needs**: the enemy gains a box collider and a breakable;
   `enemy_wreck`, `house` and `house_wreck` are new, of the imported models and built-in shapes.
   The sponsor puts a collider on the hull in `tank_body.prefab` and places houses, walls and
   posts (0251).

## Reasoning
Recording the hit on the shot keeps each row one writer's: the lives system reads the shots
that hit this step, since a hit shell is gone at that step's structural apply. Swapping in the
shell system needs no intent, because spawn and remove are structural (0190). Rejected: a hit
event queue (a second writer's queue to own for one reader); a shell that starts its sweep at
the muzzle (fires through a wall at point-blank); lives authored on the sponsor's prefab (agents
do not edit it).

## Replaces
nothing. Milestone 8 of 0268 in the game.
