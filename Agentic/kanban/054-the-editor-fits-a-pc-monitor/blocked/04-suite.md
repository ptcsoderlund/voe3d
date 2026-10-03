# 04 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING editor/src/src.md: entry `themes.h` is 305 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING ui/tests/tests.md: entry `button.c` is 366 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDINGS: 2
