# 05 — A game opens the sound device and hands the mixer to its systems
folder: game
decisions: 0168, 0175, 0237, 0257, 0265, 0266

## Change
Read `audio/include/audio/mixer.h`, `platform/include/platform/sound.h` and the program-path
call in `platform/include/platform/path.h` first.

- `cmake/voe.cmake` — `game`'s row gains `audio` (0265), its comment says why in a clause;
  `game/CMakeLists.txt` — `DEPENDS` gains `audio`.
- `game/include/game/project.h` — `voe_game_project_step` gains `voe_audio_mixer *audio`: the
  game's mixer, where a project system plays a sound (0266 point 4); NULL only in a test whose
  systems play nothing. The header's paragraph on the step says so in a clause.
- `game/include/game/steps.h`, `game/src/steps.c` — `voe_game_steps_run` takes
  `voe_audio_mixer *audio` after `window` and puts it in every step it hands out. The usage
  example in the header follows.
- `game/src/run.c` — after the world is built: the mixer made on the running program's
  folder (`voe_platform_path_program`, then its parent; a NULL program path is a mixer on
  `"."`), and `voe_platform_sound_new()`; a NULL device leaves the game silent (the report is
  already on stderr). Each frame, after the steps and the interface, a device that is open is
  pumped; a failed pump destroys it and the game goes on silent. Both are destroyed at the
  end of the run. The mixer is handed to `voe_game_steps_run`.
- `game/include/game/run.h` — `THE ORDER` names the mixer and the device, and a sentence says
  a game with no sound device runs silent.
- `game/tests/steps.c` — the calls pass `NULL` for `audio`.
- `game/game.md`, `game/include/game/game.md`, `game/src/src.md` — the entries for
  `project.h`, `steps.h`, `run.h`, `steps.c` and `run.c` mention sound in a phrase.

`examples/*/Code` builds unchanged: the step only gains a field.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder game` prints `FINDINGS: 0`.
