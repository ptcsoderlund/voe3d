# 08 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/src/src.md: entry `draw_bounce.c` is 310 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `bounce_scene.c` is 329 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `bounce_tint.c` is 349 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/include/game/game.md: entry `frame.h` is 312 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/src/src.md: entry `run.c` is 337 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDINGS: 5
