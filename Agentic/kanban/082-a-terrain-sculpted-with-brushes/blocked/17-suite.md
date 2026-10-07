# 17 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/include/3d/3d.md: does not list `brush_marker.h`
FINDING 3d/include/3d/3d.md: does not list `landscape.h`
FINDING 3d/tests/tests.md: entry `draw_markers.c` is 367 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `assets_panel.h` is 321 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `interface.c` is 348 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `view_passes.h` is 308 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `view_passes.c` is 379 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `models.h` is 329 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `undo.c` is 328 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/tests/tests.md: entry `models.c` is 323 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING check: failed: cmake -P check.cmake
    ok    standalone render
    ok    standalone scene
    ok    standalone sprite
    ok    standalone text
    ok    standalone theme
    ok    standalone ui
    ok    root configure and build
    ok    3b root release build
    ok    guard compiler
    ok    guard version
    ok    guard map
    FAIL  includes
          editor/src/assets_manage.c: includes assets/, which editor does not DEPENDS on
          editor/src/game_tree_landscapes.c: includes assets/, which editor does not DEPENDS on
          editor/src/landscape_panel.c: includes assets/, which editor does not DEPENDS on
    CMake Error at check/report.cmake:38 (message):
      check failed at: includes
    Call Stack (most recent call first):
      check/includes.cmake:68 (step_fail)
      check.cmake:59 (include)
FINDINGS: 11
