# 11 — The game's run header fits its cap
folder: game
after: none
decisions: 0168

## Change
- `game/include/game/run.h`: the header comment at the top is 61 lines; the
  cap is 60. Tighten it to 60 lines or fewer without losing a point: the
  window handed in (0291), the order of the start and of a frame, no quit key
  (0234, 0259), the constraints. Re-wrap the ragged lines in THE ORDER
  paragraph (one runs past the others' width) so the paragraph reads as one
  block; drop repeated words rather than facts. Touch no declaration and no
  comment below `#pragma once`.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game` prints no
finding for `game/include/game/run.h`.
