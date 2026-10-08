# 59 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/src/src.md: entry `shape_system.c` is 306 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `bounce_grid.c` is 544 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `shadow_lights.c` is 379 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `bounce.c` is 321 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `bounce_scene.c` is 367 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `shape.c` is 450 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/bounce_scene.c: header comment is 62 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING render/src/descriptors.c: header comment is 61 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING render/tests/tests.md: entry `bounce_volume.c` is 404 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/tests/tests.md: entry `bounce_settle.c` is 342 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/tests/tests.md: entry `bounce_read.c` is 434 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/tests/tests.md: entry `bounce_probes_scene.c` is 313 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/tests/tests.md: entry `blocked_bounce.c` is 495 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING scene/tests/tests.md: entry `transform.c` is 391 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
                  Start 175: game/starting
          175/177 Test #175: game/starting ....................   Passed    4.25 sec
                  Start 176: game/steps
          176/177 Test #176: game/steps .......................   Passed    0.02 sec
                  Start 177: game/world
          177/177 Test #177: game/world .......................   Passed    0.01 sec
          
          99% tests passed, 1 tests failed out of 177
          
          Total Test time (real) = 159.62 sec
          
          The following tests FAILED:
          	110 - 3d/bounce_scene (Failed)
          Errors while running CTest
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: tests
    Call Stack (most recent call first):
      check/tests.cmake:26 (step_fail)
      check.cmake:60 (include)
FINDINGS: 15
