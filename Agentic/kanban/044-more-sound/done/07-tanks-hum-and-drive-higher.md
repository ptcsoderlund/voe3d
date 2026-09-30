# 07 — Tanks hum, and a driven hull hums higher
folder: examples
after: 06
decisions: 0168, 0251, 0272, 0304

## Change
0304 point 8, and the feature's hum. Read `examples/tank_game/tank_game.md`,
`examples/tank_game/Code/Code.md`, `tank_hull.h`, `tank_hull_system.c`, `tank_enemy.h`,
`tank_enemy_system.c`, `audio/include/audio/sound_component.h`,
`assets/include/assets/sound.h` (the WAV forms read). Never edit `main.scene` or a `.prefab`.

- New `examples/tank_game/Assets/sounds/`: `engine.wav`, `shot.wav`, `hit.wav`,
  `explosion.wav`, mono 48 kHz PCM16, made by a throwaway python3 script run once and not
  kept. The engine is about 1 s of a low hum (fundamental near 55 Hz with a few harmonics and
  a slow wobble) whose every component makes whole periods in the file, so it loops with no
  seam; the shot a short sharp crack (~0.25 s); the hit a dull thump (~0.3 s); the explosion a
  noisy boom decaying over ~1.2 s. Peak near 0.8 of full scale, the ends of the one-shots
  faded to zero.
- `tank_hull_system.c`: a hull whose entity has no sound gets `Assets/sounds/engine.wav`,
  looping, playing, volume 0.6, pitch 1, added the way the dust emitter is. Each step it moves
  a pitch toward 1 + 0.6 × the control row's drive size (at most 1 per second, so it glides)
  and sends `voe_audio_sound_control_submit` TUNE when that differs from the row's pitch by
  more than 0.01.
- `tank_enemy_system.c`: an enemy whose entity has no sound gets the same engine, volume 0.35,
  pitch 0.85. Its removal needs nothing more: its sound goes with it (0304 point 4).
- `tank_hull.h`, `tank_enemy.h`: one point each on the hum. `Code.md`: the two entries.
  `tank_game.md`: the `Assets` entry names `sounds/`.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`python3 -c "import wave,sys;[sys.exit(1) for n in ('engine','shot','hit','explosion') if (w:=wave.open(f'examples/tank_game/Assets/sounds/{n}.wav')).getframerate()!=48000 or w.getsampwidth()!=2]"`
exits 0, and `grep -q voe_audio_sound_control_submit examples/tank_game/Code/tank_hull_system.c`
exits 0.
