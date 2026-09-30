# 13 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/tests/tests.md: entry `models.c` is 342 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/tests/tests.md: entry `steps.c` is 301 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/tests/tests.md: entry `models.c` is 313 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDINGS: 3
