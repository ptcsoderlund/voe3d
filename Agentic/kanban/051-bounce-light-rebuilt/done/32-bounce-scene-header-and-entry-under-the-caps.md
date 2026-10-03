# 32 — bounce_scene's header and index entry under the caps
folder: 3d/tests
after: none
decisions: 0168

## Change
Two findings of the whole suite, both prose; no code changes.
- `3d/tests/tests.md`: the `bounce_scene.c` entry is 393 characters, cap 300. Make it one sentence
  under 300: the probe bounce through the editor's and the game's calls, tinting, shadow foot, even
  ground, settling, and camera turns changing nothing; skips without a card. The detail it drops
  is already in the file's header.
- `3d/tests/bounce_scene.c`: the header comment above the first `#include` is 62 lines, cap 60.
  Tighten it by at least three lines without losing a check it describes: e.g. merge the TURN and
  LOOKING AWAY paragraphs' shared point (turning moves no probe; a relight shadows the sun by the
  volume's own map, 0328, 0329), or fold the pixel-projection sentence into another paragraph.
  Keep every step number, decision number and named constant (DUMMIES, BOUND, TINT, SHADOW, EVEN,
  TURNED) the body refers to. Read only the header, not the body.

## Done when
- `bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -c "FINDING 3d/tests"`
  prints 0.
