# tank_game

The top-down tank game of 0268, grown milestone by milestone (0272).

- `main.scene` — the level, the sponsor's: a ground, a camera, a sun, the player's tank, a spawner of enemy tanks and a sea.
- `water.scene` — a test scene of the water card's: main.scene's ground, camera, sun, tank and spawner, with a sloping seabed and water on each side.
- `Assets` — the sponsor's imported `.glb` models, the shell prefab the gun fires, the enemy tank the spawner makes, the breakable house, the enemy's and house's wrecks that fade a light as they appear, `images/` with a particle picture, `sounds/` with the engine hum, shot, hit and explosion.
- `Code` — the game's components and systems: control from pad or keyboard and mouse, hull, turret, gun, shells that wreck what is breakable, lives on a HUD, enemies that aim and fire, muzzle flash, the light fade of shots and wrecks, tread dust, and the camera.
