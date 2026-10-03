# 23 — The bounce_probes_scene.c entry fits the index cap
folder: render/tests
after: none
decisions: 0168

## Change
Documentation only; no code changes.

- `render/tests/tests.md`: the entry for `bounce_probes_scene.c` is 326
  characters, the cap is 300. Cut it to one sentence: probes relit a level at a
  time, checked in the picture, headless.
- `render/tests/bounce_probes_scene.c` header comment: make sure it lists the
  cases the entry drops (red wall's bounce lit side and not in shadow, red
  box's shadow unlit, even open ground, bounce strength, closed room dark,
  doorway's second bounce, lamp's bounce past its reach, settled frame
  dispatching nothing). Add only what the header does not already say.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render/tests` prints
no finding naming `bounce_probes_scene.c`.
