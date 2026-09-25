# 06 — A double position is read, written and cooked
folder: authoring
decisions: 0168, 0250

## Change
The `DOUBLE3` kind (card 01) in scene text and in the cook, so a position 100 km out survives a
save, an undo (scene texts, 0204) and Play to the last bit. `authoring` does not build at HEAD:
the switches below lack a DOUBLE3 case, and seven `scene_read`/`scene_cook`/`scene_write` tests fail
on card 03's double position; this card fixes both. Card 05's split (`field_read.h`/`.c` out of
`scene_read.c`) is already committed.

- `authoring/src/scene_write.c` — DOUBLE3 written as FLOAT3 is, `[x, y, z]`, each element the
  shortest decimal that reads back to the same double (as FLOAT64 is written today).
- `authoring/src/field_read.c` (card 05) — DOUBLE3 read as three FLOAT64 elements in one bracket;
  a file written before this card (`position = [0, 1, 0]`) still reads; `spelling()` gets its
  DOUBLE3 case.
- `authoring/src/scene_cook.c` — DOUBLE3 cooked as a brace of three `%a` hex literals without
  `f`.
- `authoring/include/authoring/scene_write.h`, `scene_cook.h` — where they list how each kind is
  spelled, DOUBLE3 is named.
- `authoring/tests/scene_write.c`, `scene_read.c`, `scene_cook.c` — follow the transform's double
  position (card 03); add: a transform at (100000.123456789, -0.05, 1e7 + 0.001) written, read
  back into a fresh world and compared bit for bit; its cooked text holds the hex literals of
  those doubles.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder authoring` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^authoring/'` passes.
3. `wc -l authoring/src/scene_read.c authoring/src/field_read.c` shows each under 800 (card 05).
