# tank_game

The top-down tank game of 0268, grown milestone by milestone (0272).

- `main.scene` — the level, the sponsor's: a ground, a camera, a sun, the player's tank and a spawner of enemy tanks.
- `Assets` — the sponsor's `.glb` models, imported in the editor, the shell prefab the gun fires and the enemy tank prefab the spawner makes.
- `Code` — the game's own components and systems: the hull that drives on WASD, the turret that aims at the pointer, the gun that fires shells, the shells that fly and vanish, and enemy tanks spawned on a timer that drive toward the player.
