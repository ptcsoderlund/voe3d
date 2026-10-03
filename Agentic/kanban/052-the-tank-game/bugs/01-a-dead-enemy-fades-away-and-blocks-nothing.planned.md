# 01 — A dead enemy fades away and blocks nothing

## Seen
"dead body enemies should have no collider and they should fade away into nothing."

A destroyed enemy tank leaves a wreck that stays in the level for good and is still solid: the player's
tank and shells hit it.

## Expected
From the moment an enemy tank dies, its wreck has no collider: tanks drive through it and shells fly
through it. The wreck lies still for 2 seconds, then turns see-through over 1 second until it is
gone, and it leaves the world entirely. It looks the same in the editor's Play and in the shipped
game.

Only enemy tanks do this. A destroyed house, crate or other breakable keeps its solid wreck, as now,
so the level still shows what was blown up.

## How to reproduce
1. Open `examples/tank_game` and press Play, or run the shipped game, and start from the menu.
2. Destroy an enemy tank.
3. Drive the player's tank into its wreck and shoot at it: the tank stops against it and the shell
   hits it. The wreck never goes away.
4. Destroy a crate or another breakable: its wreck stays and stays solid. That part is right.
