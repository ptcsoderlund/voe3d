# 15 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/src/src.md: entry `draw_shadows.c` is 330 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/src/src.md: entry `models_landscape.c` is 330 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `draw_foliage.c` is 385 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING assets/tests/tests.md: entry `landscape.c` is 322 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `assets_drag.h` is 309 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `inspector.c` is 301 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `sculpt.h` is 325 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `models.h` is 312 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/assets_drag.h: header comment is 64 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING game/include/game/game.md: entry `models.h` is 325 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/include/render/render.md: entry `device.h` is 338 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/src/descriptors.c: header comment is 62 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDINGS: 12
