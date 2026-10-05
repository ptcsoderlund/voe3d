# 10 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING dev/src/src.md: entry `main.c` is 311 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `main.c` is 346 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/tests/tests.md: entry `card.c` is 379 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDINGS: 3
