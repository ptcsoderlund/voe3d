# 22 — The descriptors.c header fits the cap
folder: render/src
after: none
decisions: 0168

## Change
`render/src/descriptors.c`: its header comment is 62 lines, cap 60. Comments only; no code
changes.

- Move the sentence "Every per-slot buffer stays mapped for its whole life (see
  build_mapped)" out of the POOL paragraph into the comment above `build_mapped` in the same
  file (add one if there is none).
- Fold the two-line push-constant paragraph (a mesh draw's object number, the element
  pipeline's surface transform through the same range, see device.c) into one line.
- Keep the thirteen-binding table whole.

Measure with `bash ~/.claude/skills/checks/scripts/checks.sh --header-lines render/src/descriptors.c`.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines render/src/descriptors.c`
does not print "over cap".
