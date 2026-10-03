# 21 — The draw_shadows.c entry fits the index cap
folder: 3d/src
after: none
decisions: 0168

## Change
Documentation only; no code changes.

- `3d/src/src.md`: the entry for `draw_shadows.c` is 311 characters, the cap is
  300. Cut it to one sentence naming what the file does: the shadow passes
  (cascades, the point-shadow pass, the probe bounce) as one call.
- `3d/src/draw_shadows.c` header comment: make sure it carries what the entry
  drops: why this is its own call and who casts and bounces. Add it there only
  if the header does not already say it.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/src` prints no
finding naming `draw_shadows.c`.
