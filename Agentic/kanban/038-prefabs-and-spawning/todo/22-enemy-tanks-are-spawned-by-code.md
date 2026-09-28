# 22 — Enemy tanks are a prefab, spawned by code
folder: examples
decisions: 0168, 0283, 0272, 0256

## Change
Needs card 21. The enemies of `feature.md` step 5. Every file is under `examples/tank_game/`;
read `Code/tank_shell.h`, `Code/tank_shell_system.c` and `Code/project.c` as the pattern.

- `Code/tank_enemy.h`, `Code/tank_enemy_system.c` (new): "Tank / Enemy": `speed` (m/s, default 2)
  and `life` (seconds, default 20, counted down). Each enemy drives along its own −Z through one
  transform intent a step and is removed with `voe_game_project_remove` (its turret with it) when
  `life` reaches zero. Room for 64.
- `Code/tank_spawner.h`, `Code/tank_spawner_system.c` (new): "Tank / Spawner": `prefab` (CHAR 64,
  default `enemy_tank`), `every` (seconds, default 4), `most` (default 6) and `wait` (counted down
  by the system). While the world holds fewer than `most` `tank_enemy` rows and `wait` is at or
  below zero, it spawns `prefab` at the spawner's world position and rotation and `wait` becomes
  `every`. Room for `VOE_GAME_WORLD_AUTHORED`.
- `Code/project.c`: registers both; runs the spawner and then the enemies after the shells.
- `Assets/enemy_tank.prefab` (new): `[1]` "Enemy" with a transform, `tank_enemy` and the model
  `Assets/tank_body.glb`; `[2]` "Enemy turret" under it with the model `Assets/tank_head.glb` and
  the transform the turret `[5]` has in `main.scene`. No hull or turret component: the player's
  keys and mouse drive only the player.
- `main.scene`: a new `[6]` "Spawner" with a transform at (0, 0, -15) turned to face +Z and a
  `tank_spawner` section with the defaults.
- `Code/Code.md`, `tank_game.md`: the new files and what the game now does.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" && test
-s "$d/t.png"` exits 0. The human's: Play: an enemy tank appears at the spawner every four
seconds, up to six, and drives toward the player.
