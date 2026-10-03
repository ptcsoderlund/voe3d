# 22 — The descriptors.c entry fits the index cap
folder: render/src
after: none
decisions: 0168

## Change
Documentation only; no code changes.

- `render/src/src.md`: the entry for `descriptors.c` is 327 characters, the cap
  is 300. Cut it to one sentence: everything the shader reads and the one
  layout that describes it.
- `render/src/descriptors.c` header comment: make sure it carries what the
  entry drops: the eleven bindings in one set, the camera buffer's block per
  pass, the object, element, point light and light bin buffers, the shadow and
  point shadow maps per frame slot, and each probe volume's sum and moments
  with their two samplers. Add only what the header does not already say.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render/src` prints no
finding naming `descriptors.c`.
