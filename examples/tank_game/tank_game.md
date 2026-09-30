# tank_game

The top-down tank game of 0268, grown milestone by milestone (0272).

- `main.scene` — the level, the sponsor's: a ground, a camera, a sun, the player's tank and a spawner of enemy tanks.
- `Assets` — the sponsor's `.glb` models, imported in the editor, the shell prefab the gun fires, the enemy tank prefab the spawner makes, the breakable house with the enemy's and house's wrecks, `images/` with a particle picture, and `sounds/` with the engine hum, shot, hit and explosion.
- `Code` — the game's components and systems: control from pad or keyboard and mouse, hull, turret, gun, shells that wreck what is breakable, lives on a HUD, enemies that aim and fire, muzzle flash and tread dust, and the camera.
