# 65 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
                |         ^
          /home/ptcsoderlund/Projekt/voe3d/render/tests/bounce_read.c:228:3: note: Returning without writing to '*named'
            228 |                 return false;
                |                 ^
          /home/ptcsoderlund/Projekt/voe3d/render/tests/bounce_read.c:305:2: note: Returning from 'draw_frame'
            305 |         draw_frame(scene, 1, 0, named, NULL, arena);
                |         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/render/tests/bounce_read.c:306:26: note: The left operand of '==' is a garbage value
            306 |         VOE_TEST_CHECK(named[0] == VOE_RENDER_NO_BOUNCE);
                |                        ~~~~~~~~ ^
          /home/ptcsoderlund/Projekt/voe3d/cmake/../testing/include/testing/test.h:96:18: note: expanded from macro 'VOE_TEST_CHECK'
             96 |         voe_test_check((expr) ? 1 : 0, __FILE__, __LINE__, #expr)
                |                         ^~~~
          1 warning generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 1
