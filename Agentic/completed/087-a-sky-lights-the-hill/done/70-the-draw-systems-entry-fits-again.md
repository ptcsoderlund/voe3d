# 70 — The draw system's index entry fits its cap again
folder: 3d/include/3d
after: none
decisions: 0168

## Change
- `3d/include/3d/3d.md`: the `draw_system.h` entry is 311 characters, cap 300. Shorten it to
  one sentence of at most 300 characters: keep that it draws the world into the pass the loop
  opened after the shadow passes and probe bounce, with the frame's lights, blockers, sky and
  haze; drop the detail lists (fading model see-through, each light's disc, the haze's top
  above the eye).
- `3d/include/3d/draw_system.h`: read its header comment only. Any point dropped from the
  entry that the header does not already make is added there as a phrase. Comments only; no
  code or signature changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/include/3d` prints `FINDINGS: 0`.
