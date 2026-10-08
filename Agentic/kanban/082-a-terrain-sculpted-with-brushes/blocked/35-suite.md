# 35 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: cmake -P check.cmake
                  Start 174: game/starting
          174/176 Test #174: game/starting ....................   Passed    0.87 sec
                  Start 175: game/steps
          175/176 Test #175: game/steps .......................   Passed    0.03 sec
                  Start 176: game/world
          176/176 Test #176: game/world .......................   Passed    0.01 sec
          
          99% tests passed, 1 tests failed out of 176
          
          Total Test time (real) =  83.58 sec
          
          The following tests FAILED:
          	 64 - render/best_practices (Failed)
          Errors while running CTest
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: tests
    Call Stack (most recent call first):
      check/tests.cmake:26 (step_fail)
      check.cmake:60 (include)
FINDINGS: 1
