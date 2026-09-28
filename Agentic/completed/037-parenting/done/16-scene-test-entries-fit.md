# 16 — The scene tests' index entry fits the cap
folder: scene/tests
decisions: 0168

## Change
`scene/tests/tests.md`: the entry `transform.c` (391 characters) is over the 300-character entry
cap. Shorten it to one sentence under 300 characters that says what the file proves. Open the
header comment of `scene/tests/transform.c`; any point the entry drops that the header does not
already make (a child composing under its parent and relative undoing it, the far millimetre and
zero scale cases, a child blending along its blended parent) goes into that header. No code
changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder scene/tests` prints `FINDINGS: 0`.
