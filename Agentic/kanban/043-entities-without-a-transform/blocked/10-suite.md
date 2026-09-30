# 10 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING authoring/tests/tests.md: entry `scene_read.c` is 308 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `scene.c` is 366 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/tests/tests.md: entry `project.c` is 342 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING scene/tests/tests.md: entry `parent.c` is 361 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING scene/tests/tests.md: entry `identity.c` is 367 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDINGS: 5
