# 20 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/src/src.md: entry `draw_system.c` is 312 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/view_passes.h: header comment is 61 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING game/tests/tests.md: entry `frame.c` is 304 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/tests/frame.c: header comment is 62 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING render/tests/tests.md: entry `exposure.c` is 308 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/tests/tests.md: entry `sky_light.c` is 320 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING scene/include/scene/scene.md: does not list `picture_component.h`
FINDING scene/include/scene/scene.md: does not list `picture_system.h`
FINDINGS: 8
