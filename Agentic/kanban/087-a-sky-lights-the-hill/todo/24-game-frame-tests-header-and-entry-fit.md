# 24 — The game frame test's header and entry fit their caps
folder: game/tests
after: none
decisions: 0168

## Change
- `game/tests/frame.c`: its header comment is 62 lines, cap 60. Bring it to at most 60: the why
  of a single test moves to a comment above that test's function; what the file does, how it
  runs and its constraints stay. Comments only; no code changes.
- `game/tests/tests.md`: the `frame.c` entry is 304 characters, cap 300. Shorten it to one
  sentence of at most 300 characters; the list of cases it names may be summarised, since the
  header carries them.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/tests` prints `FINDINGS: 0`.
