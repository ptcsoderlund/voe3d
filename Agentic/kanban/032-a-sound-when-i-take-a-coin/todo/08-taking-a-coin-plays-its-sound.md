# 08 — Taking a coin plays its sound
folder: examples/coin_game
decisions: 0168, 0251, 0260, 0266
read: feature.md

## Change
Read `audio/include/audio/mixer.h` and `game/include/game/project.h` (the step's `audio`).
Never edit `main.scene` (0251): its coins have no `sound` key, so they read the default.

- New `examples/coin_game/pickup.wav` — the starter pickup sound, made once with a throwaway
  `python3` script using the standard `wave` module (the script is not committed): 44100 Hz
  mono 16-bit (so the mixer's resampling is used), about 0.3 s, a short rising two-tone
  chime (about 988 Hz for 0.07 s, then about 1319 Hz fading out), peak near half scale, a few
  milliseconds of fade at each edge so it does not click.
- `examples/coin_game/Code/coin.h` — `COIN_FIELDS` gains `F(char, sound, CHAR, 128)`: the
  sound a take plays, a path relative to the project, empty for none. `coin_system_run`
  takes `voe_audio_mixer *audio` after the world. The header says a take plays the coin's
  sound and that two takes overlap.
- `examples/coin_game/Code/coin_system.c` — the default row gains `.sound = "pickup.wav"`;
  each take calls `voe_audio_mixer_play(audio, coin->sound)` once, where the Shape is removed
  (a put-back plays nothing).
- `examples/coin_game/Code/project.c` — passes `step->audio`.
- `examples/coin_game/Code/Code.md` — the `coin.h` and `coin_system.c` entries mention the sound.
- `examples/coin_game/coin_game.md` — an entry for `pickup.wav`: the starter sound, shipped
  beside the game (0266 point 3), swappable in the Inspector on each coin.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/coin_game` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0,
   `$p/Build/editor/loaded/project-1.so` exists and `cat $p/err` shows no line about `sound`
   or `audio`; then `cmake -S $p/Build/game -B $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target game`
   exits 0 and `$p/Build/debug/pickup.wav` exists; then
   `cmake -S $p/Build/game -B $p/Build/release -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build $p/Build/release --target game && cmake --install $p/Build/release --prefix $p/out`
   exits 0 and `$p/out/pickup.wav` exists.
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's: `feature.md`'s `## How to test`, steps 1 to 6, in the editor on Linux
   (step 4 with another `.wav` put in the project folder; step 6 as in 031).
