# 10 — A game's run holds its sound device without a double pointer
folder: game/src
decisions: 0168, 0265, 0266

## Change
Only `game/src/run.c`. `run_frames` takes `voe_platform_sound **sound` (line 53) so a device
whose pump fails can be destroyed and set to NULL for the caller; the checks allow one level
of dereference only.

- Add a file-local `struct run_sound` holding `voe_audio_mixer *mixer` and
  `voe_platform_sound *device`, with a short comment: the run's sound, the device NULL when
  there is none or it went away.
- `run_frames` takes `struct run_sound *sound` in place of its `mixer` and `sound` parameters;
  on a failed pump it destroys `sound->device` and sets it to NULL. Where it hands the mixer to
  the steps it uses `sound->mixer`.
- The caller (the run function below it) fills one `struct run_sound` from the mixer and
  device it already makes, passes its address, and destroys `sound.device` on the way out as
  it destroyed `sound` before. Behaviour is unchanged.
- The comment above `run_frames` still says a failing device is destroyed and set to NULL and
  the game goes on silent; adjust its wording to the struct if needed.

No public header changes; `src/src.md`'s `run.c` entry stays as it is unless it names the
parameters.

## Done when
1. `grep -nE '\*[[:space:]]*\*' game/src/run.c` prints nothing.
2. `bash ~/.claude/skills/checks/scripts/checks.sh --folder game/src` prints `FINDINGS: 0`
   (it builds `game` and runs every `game/` test).
