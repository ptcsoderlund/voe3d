# 79 — The assets parts test entry is one sentence
folder: assets/tests
after: none
decisions: 0168

## Change
`assets/tests/tests.md`: the entry `parts.c` is 324 characters, over the
300-character cap.

Cut it to one sentence under 270 characters: a parts file's entries and
levels, shares and swaps included, round-tripped byte for byte, defaults for
what is absent, and bad input refused MALFORMED. Details that may be dropped
(the middle entry empty; an empty section read as count 0; the old swap word
filling only absent swaps; the list of bad keys, paths, levels, shares and
swaps) are already in `assets/tests/parts.c`'s header comment (14 lines); add
only what it lacks.

Comments and index only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `assets/tests/tests.md`.
