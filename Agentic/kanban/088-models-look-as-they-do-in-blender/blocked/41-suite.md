# 41 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/tests/tests.md: entry `draw_material.c` is 362 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING assets/include/assets/assets.md: does not list `model_material.h`
FINDING assets/include/assets/assets.md: does not list `parts.h`
FINDING assets/tests/tests.md: entry `material.c` is 305 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `session.c` is 326 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `assets_panel.c` is 356 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `assets_manage.h` is 396 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `scene.h` is 305 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/tests/tests.md: entry `models.c` is 358 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/src/src.md: entry `draw.c` is 330 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDINGS: 10
