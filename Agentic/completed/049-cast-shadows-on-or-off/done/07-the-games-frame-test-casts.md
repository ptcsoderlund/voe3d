# 07 — The game's frame test casts
folder: game
after: 02, 03, 04
decisions: 0168, 0324

## Change
0324 point 2: literals cast nothing, so the shadow case of the game's frame
test sets its flags. Read `game/tests/frame.c` and `game/tests/tests.md`.

- `game/tests/frame.c`: the light's literal (around line 100) and the
  cube's shape literal (around line 108) set `cast_shadows = true`, so
  `shadow_case` still opens the shadow passes and draws a caster. The
  header comment says both cast. Any other test in `game/tests/` that
  fails because its light or shape no longer casts is fixed the same way.
- `game/tests/tests.md` only if frame.c's entry no longer says what it
  does.

## Done when
`ctest --test-dir build/debug -R '^game/'` passes; `game/frame` skips on a
machine without a graphics card, and its build is then the proof.
