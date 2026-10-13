# 62 — The game frame test's header and entry fit their caps
folder: game/tests
after: none
decisions: 0168

## Change
- `game/tests/frame.c`: its header comment is 64 lines, cap 60. Bring it to at most 60: the
  why of a single case (the haze case's point 0417 detail, the blocker and Direct cases'
  read-back points are candidates) moves to a comment above that case's test function; what
  the file does, how it runs, that it skips without Vulkan and that the haze case needs no
  device stay in the header. Comments only; no code changes.
- `game/tests/tests.md`: the `frame.c` entry is 321 characters, cap 300. Shorten it to one
  sentence of at most 300 characters; summarise the list of cases (the header names them),
  keeping that it skips without a graphics card.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/tests` prints `FINDINGS: 0`.
