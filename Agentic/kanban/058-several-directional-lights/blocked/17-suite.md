# 17 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/include/3d/draw_system.h: header comment is 61 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING 3d/src/src.md: entry `draw_system.c` is 312 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/src/src.md: entry `draw_light_blockers.c` is 349 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/src/src.md: entry `draw_bounce.c` is 305 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `view_passes.h` is 310 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/src/src.md: entry `frame.c` is 341 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/tests/tests.md: entry `frame.c` is 312 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/src/src.md: entry `pass.c` is 330 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/src/descriptors.c: header comment is 61 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING Agentic/tech-lead/system.md: over 60 lines at 100 columns; it is a map, not a description (--system-cap to raise)
FINDINGS: 10
