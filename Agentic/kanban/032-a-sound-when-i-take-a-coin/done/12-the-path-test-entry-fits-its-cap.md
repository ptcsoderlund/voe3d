# 12 — The path test's entry fits its cap
folder: platform/tests
decisions: 0168

## Change
Only `platform/tests/tests.md`. Its `path.c` entry is 348 characters; the cap is 300.

- Shorten the `path.c` entry to one sentence under 300 characters: join, parent and name for
  the platform it runs on, resolving, and the program's own path. The detail it drops (roots
  and trailing separators, "." idempotent, a made-up name NULL, the non-ASCII folder byte for
  byte) is already in `platform/tests/path.c`'s header comment; read only that header to
  confirm, and do not change the file.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder platform/tests` prints `FINDINGS: 0`.
