# 25 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING game/tests/tests.md: entry `frame.c` is 302 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/include/render/render.md: entry `device.h` is 313 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDINGS: 2
