# 49 — The cook test suppresses its one fopen
folder: authoring/tests
after: 43
decisions: 0168, 0413

## Change
Bug 05: `authoring/tests/scene_cook.c`'s `write_file` (about line 183) calls `fopen`, deprecated by the
Windows headers. `authoring` does not link `platform`, so it keeps `fopen` (0413 point 5) and the one call
carries `#pragma clang diagnostic push` / `ignored "-Wdeprecated-declarations"` / `pop`, in the shape
`game/tests/frame.c` uses around line 188: a comment naming ADR-0159 and 0413, and that the replacement,
Annex K's `fopen_s`, is not on Linux. Nothing wider is suppressed. No index change.

## Done when
- `grep -c 'diagnostic ignored "-Wdeprecated-declarations"' authoring/tests/scene_cook.c` prints 1.
- Test `authoring/scene_cook` passes.
