# 18 — The model test's index entry is one sentence
folder: assets/tests
after: none
decisions: 0168

## Change
`assets/tests/tests.md`: the entry for `model.c` is 305 characters, cap 300.
Cut it to one sentence under 260 characters: a hand-built `.glb` read back
into its arrays, its fields read or defaulted, and a working file corrupted
a byte at a time. The details dropped (two textures over one picture,
COLOR_0 forms, names, sides, unlit, emission, repeat, a long name cut, node
names) must be in `assets/tests/model.c`'s header comment; add there any it
does not already say, keeping the header within its cap. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `assets/tests/`, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines assets/tests/model.c`
prints no "over cap".
