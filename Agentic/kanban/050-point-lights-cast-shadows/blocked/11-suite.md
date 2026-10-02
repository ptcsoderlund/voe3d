# 11 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/tests/tests.md: entry `point_lights.c` is 369 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/view_passes.h: header comment is 61 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING game/tests/tests.md: entry `frame.c` is 345 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/include/render/render.md: entry `device.h` is 344 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/src/src.md: does not list `point_shadow_faces.c`
FINDING render/tests/tests.md: entry `point_shadows.c` is 464 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/tests/tests.md: entry `point_lights.c` is 351 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING scene/tests/tests.md: entry `point_light.c` is 357 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDINGS: 8
