# 08 — Shots, hits and explosions are heard where they happen
folder: examples
after: 07
decisions: 0168, 0251, 0272, 0304

## Change
The feature's last sentence. Read `examples/tank_game/Code/Code.md`, `tank_gun.h`,
`tank_gun_system.c`, `tank_enemy_system.c`, `tank_shell.h`, `tank_shell_system.c`,
`game/include/game/project.h` (the step's `audio`), `audio/include/audio/mixer.h`
(`voe_audio_mixer_play_at`). Never edit `main.scene` or a `.prefab`.

- `tank_gun_system.c`: each shot fired plays `Assets/sounds/shot.wav` at the muzzle's world
  position through the step's mixer.
- `tank_enemy_system.c`: each shot an enemy fires does the same at its muzzle, at the same call.
- `tank_shell_system.c`: a shell that hits plays `Assets/sounds/hit.wav` at the hit point;
  when the hit swaps a breakable for its wreck, it plays `Assets/sounds/explosion.wav` at the
  broken thing's position instead.
- `tank_gun.h`, `tank_shell.h`: one point each. `Code.md`: the three entries.

The human, after the build, with headphones: open the tank game and press Play.
1. The engine hums with no gap in its loop; driving raises its pitch, stopping lowers it.
2. Shoot an enemy on the left of the screen: the explosion is in the left ear. Then the right.
3. A far explosion is quieter than a near one.
4. Destroy an enemy whose engine hums: its hum stops with it.
5. Stop the game while sounds play: everything goes silent at once.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`grep -q voe_audio_mixer_play_at examples/tank_game/Code/tank_gun_system.c && grep -q
voe_audio_mixer_play_at examples/tank_game/Code/tank_enemy_system.c && grep -q
explosion.wav examples/tank_game/Code/tank_shell_system.c` exits 0. The rest is the human's
steps above.
