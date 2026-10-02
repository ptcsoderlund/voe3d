# 09 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/include/3d/3d.md: entry `draw_system.h` is 328 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `shadows.c` is 321 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING scene/tests/tests.md: entry `light.c` is 357 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING Agentic/tech-lead/decisions/implemented/0320-a-point-light-is-a-scene-row-with-a-reach-and-a-flash-shaded-from-cpu-binned-bitmasks.md: stray; not a place the workflow names
FINDING Agentic/tech-lead/decisions/implemented/0322-falloff-bends-the-reach-curve-and-the-tank-game-fades-its-own-lights.md: stray; not a place the workflow names
FINDING Agentic/tech-lead/decisions/implemented/0323-a-muzzle-light-is-a-child-at-the-flash.md: stray; not a place the workflow names
FINDINGS: 6
