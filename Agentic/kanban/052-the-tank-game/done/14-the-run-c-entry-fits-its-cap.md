# 14 — The `run.c` entry in src.md fits its cap
folder: game/src
after: none
decisions: 0168

## Change
Text only. Read `game/src/src.md` and the header comment of `game/src/run.c`.

- `game/src/src.md`: the `run.c` entry is over the 300-character entry cap.
  Cut it to one sentence: the run's steps in the handed window, and the only
  file naming the cooked scene, the cooked prefabs and the project's entry
  points. The interface's asks, the mixer paused, and the device pumped or
  silent leave the entry.
- `game/src/run.c`: its header comment gains, as one or two short points, what
  the entry drops and the header lacks: the interface run each frame and may
  end the run; a restart asked makes the world again in its own arena; the
  mixer pauses with the run. Header only; no code changes.

## Done when
`awk '/^- .run\.c./{ok = length($0) < 290} END{exit !ok}' game/src/src.md`
exits 0.
