# tank_game

The top-down tank game of 0268, grown milestone by milestone (0272).

- `main.scene` — the level, the sponsor's: a ground, a camera, a sun, the player's tank and a spawner of enemy tanks.
- `Assets` — the sponsor's `.glb` models, imported in the editor, the shell prefab the gun fires, the enemy tank prefab the spawner makes, and the breakable house with the enemy's and house's wrecks.
- `Code` — the game's components and systems: one control row from pad or keyboard and mouse; the hull, the aiming turret, the gun, shells that stop where they hit and wreck what is breakable, the player's lives a hit costs, shown in a HUD, enemies spawned on a timer, and the camera.
