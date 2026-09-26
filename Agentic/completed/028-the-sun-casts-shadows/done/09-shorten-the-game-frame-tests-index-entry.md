# 09 — Shorten the game frame test's index entry
folder: game/tests
decisions: 0168

## Change
`checks.sh --all` finds `game/tests/tests.md`'s `frame.c` entry at 332 characters against a cap
of 300 (left by card 07). Only `game/tests/tests.md` changes.

- `game/tests/tests.md` — the `frame.c` entry says, in under 300 characters: two headless
  frames of a camera, light and cube, with and without the light, and a capsule casting onto
  the cube with every shadow pass fitting; skips without a graphics card. The detail it drops
  (lag 0, the collider replace drained, the PNG) is already in `game/tests/frame.c`'s header
  (TWO CASES, TWO FRAMES, THE SHADOW CASE); read that header to confirm, change no code.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder game/tests` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R "^game/"` exits 0 after `cmake --build --preset debug`.
