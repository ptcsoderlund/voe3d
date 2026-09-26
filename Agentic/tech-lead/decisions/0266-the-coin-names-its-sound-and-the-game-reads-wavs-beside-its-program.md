# 0266 — The coin names its sound, and the game reads `.wav` files beside its program
date: 2026-09-26
by: planner

## Decision
For 032, filling in what the feature and 0265 leave to the planner:

1. **The coin carries its sound.** `Coin` gains `sound`, a CHAR field of 128 bytes: a path
   relative to the project folder, default `pickup.wav`. Empty is no sound, silently. There is
   no separate sound component; a pickup sound belongs to what is picked up.
2. **WAV only**: RIFF/WAVE, PCM 16-bit or IEEE float 32-bit (plain or extensible header), one
   or two channels, any rate. Decoded in `assets/sound.h` to interleaved float.
3. **The game reads its files beside its program.** `platform/path.h` gains the running
   program's own path; the game resolves a sound's path against that program's folder, in Play
   (`Build/debug/`) and shipped (`Build/ship/<name>/`) alike. `cmake/game.cmake` copies every
   `.wav` of the project, outside `Build/` and `Cache/`, to the same relative path beside the
   program when the game builds, and installs them the same way into the shipped folder. The
   project folder is the game tree's grandparent (`<project>/Build/game/`, 0235).
4. **The mixer is called, not sent an intent.** `audio/mixer.h`: a mixer made with the folder
   paths are read from; `play(path)` loads a file on its first ask (read, decode, converted to
   48 kHz stereo by linear interpolation), keeps it for later plays, and starts a new voice.
   A file that fails is reported once, naming the path, and stays silent. 16 voices; a play
   when all are busy takes the oldest. `mix` sums voices into float frames clamped to ±1;
   `pump` hands the device what it has room for. The project reaches the mixer through a new
   `audio` field on `voe_game_project_step`, as the interface reaches `ui`.
5. **Pushed once a frame, 50 ms ahead.** The device is filled from the game's own frame loop,
   no audio thread: `platform/sound.h` answers how many frames it takes now without holding more
   than 2400 (50 ms) queued, and an underrun is recovered quietly. On Windows `ole32` is loaded
   at run time as ALSA is on Linux, so the build links nothing new.
6. **The editor links `audio`** (0265 grants the edge) so a project library that plays sounds
   binds in the editor; the editor itself plays nothing, as project systems do not run there.

## Reasoning
A path in the component is what the Inspector, the scene file and the cook already carry
(CHAR fields). Reading beside the program gives Play and the shipped game one rule and no
argument to pass; copying every project `.wav` avoids a list the cook would have to learn to
build. A direct call keeps the first sound to one line in the coin system; the mixer is a
service like `ui`, and its voices are only ever written inside `audio`. Rejected: an intent
queue for sounds (a registration and a drain for one caller), an audio thread (locking in the
mixer for a sound that tolerates 50 ms), a working-directory rule (breaks when the shipped
program is started from elsewhere).

## Replaces
nothing
