# 10 — The game tree's header and index entry fit their caps
folder: editor/src
after: none
decisions: 0168, 0291

## Change
Two size findings from the suite, both in `editor/src`, both left by card 06. Prose only; no
code changes. Files: `editor/src/game_tree.h`, `editor/src/game_tree.c`, `editor/src/src.md`.

- `game_tree.h`: the header comment above `#pragma once` is 61 lines, cap 60. Tighten it to at
  most 60 lines without losing a point: rewrap paragraphs (the layout paragraph has a short
  broken line after `prefabs.c;`) or shorten wording. Keep every ADR reference and the
  main.c-carries-the-game-window paragraph (0291 point 3, 0236).
- `src.md`: the `game_tree.c` entry is 307 characters, cap 300; it is also wrapped past the
  file's line width. Make it one sentence under 300 characters, wrapped like its neighbours.
  Drop the least telling item (e.g. "each argument list in one struct" or "the name and engine
  path escaped for where they go"); what is dropped must be said in `game_tree.c`'s own header
  comment, so check that header says it and add a phrase there if not.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints `FINDINGS: 0`.
