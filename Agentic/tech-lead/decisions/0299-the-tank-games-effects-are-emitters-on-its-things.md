# 0299 — The tank game's effects are emitters on its things
date: 2026-09-30
by: planner

## Decision
For 042:
1. **The wrecks** (`enemy_wreck.prefab`, `house_wreck.prefab`) carry two emitters on parts they
   already have, one row of a type per entity: fire on the root (glow, orange to red,
   burst 30, no rate, 0.6 s) and smoke on the first
   child part (lit, grey, rise, burst 16, then 3 a
   second, 2.5 s, growing and fading). They play when spawned, so a hit explodes.
2. **Muzzle flash**: an emitter on the gun's entity at the muzzle offset, pointing along the
   barrel, not playing, glow, burst 12, 0.12 s. The enemy prefab carries its own. The player's
   is added at run time by the gun system when the gun's entity has no emitter, because agents
   do not edit `tank_body.prefab` (0272, 0295). Each shot fires a burst control.
3. **Tread dust**: the hull system adds a dust emitter to each `tank_hull` without one (at the rear,
   brown, lit, rise, 12 a second, 1 s) and plays it while the drive is past 0.1 and stops it
   otherwise, sending the control only when that changes.
4. **Enemies** carry their own dust on the hull, always playing, since they always drive.

## Reasoning
Every effect is data on the thing it belongs to, so the Inspector can tune the prefabs, and
code only says when. Adding rows at run time is 0295's precedent for the sponsor's prefab.
Rejected: spawning a short-lived effect prefab per shot (six entities a second, with no one to
remove them).

## Replaces
Nothing.
