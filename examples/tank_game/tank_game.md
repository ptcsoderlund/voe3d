# tank_game

The top-down tank game of 0268, whole (0272, 0334): it scrolls along −Z, enemies come in the sponsor's waves, a score and lives, a goal to win at, and a menu, pause and game over by pad or keyboard.

- `main.scene` — the level, the sponsor's: a ground, a camera, a sun, the player's tank, a sea, spawners along it as waves and a Tank / Goal at its end.
- `water.scene` — a test scene of the water card's: main.scene's ground, camera, sun, tank and spawner, with a sloping seabed and water on each side.
- `Assets` — the sponsor's imported `.glb` models, the shell prefab the gun fires, the enemy tank the spawner makes, the breakable house, the enemy's and house's wrecks that fade a light as they appear, `images/` with a particle picture, `sounds/` with the engine hum, shot, hit and explosion.
- `Code` — the game's components and systems: control from pad or keyboard and mouse, hull, turret, gun, shells that wreck what is breakable, lives, enemies, the scroll, spawners as waves, score, goal, the round's state and the menu, light fades, dust and the camera.
