# 0323 — A muzzle light is a child entity at the flash
date: 2026-10-02
by: planner

## Decision
For 048 bug 02:
1. **Its own entity.** A shot's light is not on the gun or turret, whose place is their pivot. It
   is a child entity, parented to the gun or turret, with a transform whose local position is the
   muzzle flash emitter's offset (the emitter's offset is in the same local space), identity
   rotation, scale 1. It carries the point light and its `tank_light_fade`; the barrel carries the
   light as the turret turns and the tank drives (0281).
2. **The enemy.** `enemy_tank.prefab` gains entity 3, "Enemy muzzle light", under the turret at the
   turret emitter's offset; the turret loses its light and fade.
3. **The player.** The gun system makes the child at run time, never saved (as 0299 point 2's
   emitter): `voe_ecs_entity_create`, then structural adds in order: transform, parent row naming
   the gun, point light, fade — the order a prefab spawn queues them. A refused add queues its
   destroy, to try again next step.
4. **Finding it.** `tank_light_fade_under(world, parent, &out)`: the first fade row whose parent row
   names `parent`. The gun uses it to know it has one, and both shooters to restart its fade.
5. Wrecks keep their light on their own entity: their place is the explosion's.

## Reasoning
A point light shines from its transform's world position only (0320), so an offset field on the
light would be an engine change for one game's need; a child is what parenting is for and needs no
new engine code. One finder serves both shooters.

## Replaces
0322 point 6 in where the shot's fade row sits: on the muzzle light, not the gun or turret.
