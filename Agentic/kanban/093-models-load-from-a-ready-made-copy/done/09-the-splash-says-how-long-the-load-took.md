# 09 — The splash says how long the load took
folder: game
after: 08
decisions: 0168, 0470, 0370, 0346

## Change
0470 point 7: every splash wait ends with "Loaded in N.N s" for one second.

- `game/include/game/progress.h`: add `void voe_game_progress_loaded(double seconds, char *buffer, size_t size)`.
  It writes "Loaded in N.N s", one decimal, cut to size with its terminator. Use only the font's
  characters (`text/font.h`, see `starting.h`'s constraint).
- `game/src/progress.c`: the function.
- `game/include/game/starting.h`: the wait's comment, the paragraph on its two threads, and the
  declaration's comment say three things:
  - the wait measures from its own start to the work's end;
  - on a true answer it draws that line for 1 s, polling as it does, so a close still ends it at once;
  - a headless app draws it once and does not hold, as it never waits.
- `game/src/starting.c` (276 lines): the clock (`platform/clock.h`), the line and the hold. A false answer
  or a closing window skips the line.
- `game/tests/starting.c`: `progress_lines` gains `voe_game_progress_loaded(2.46, ...)` as
  "Loaded in 2.5 s", and 0.04 as "Loaded in 0.0 s". The `game/tests/tests.md` entry names it.

## Done when
`ctest --test-dir build/debug -R '^game/starting$'` passes with the loaded line checked.
