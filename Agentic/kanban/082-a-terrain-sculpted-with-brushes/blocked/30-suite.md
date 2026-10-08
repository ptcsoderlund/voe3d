# 30 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING platform/src/src.md: entry `file_win32.c` is 351 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING check: failed: cmake -P check.cmake
                  Start 175: game/steps
          175/176 Test #175: game/steps .......................   Passed    0.03 sec
                  Start 176: game/world
          176/176 Test #176: game/world .......................   Passed    0.01 sec
          
          98% tests passed, 3 tests failed out of 176
          
          Total Test time (real) =  83.56 sec
          
          The following tests FAILED:
          	 18 - platform/file (Failed)
          	 64 - render/best_practices (Failed)
          	 87 - render/mips (Failed)
          Errors while running CTest
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: tests
    Call Stack (most recent call first):
      check/tests.cmake:26 (step_fail)
      check.cmake:60 (include)
FINDINGS: 2
