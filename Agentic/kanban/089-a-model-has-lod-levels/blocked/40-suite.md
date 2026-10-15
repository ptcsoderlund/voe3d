# 40 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING editor/src/src.md: entry `session.h` is 307 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `interface.h` is 330 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/include/game/models.h: header comment is 63 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING game/src/src.md: entry `models.c` is 321 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/tests/tests.md: entry `models.c` is 341 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
          /home/ptcsoderlund/Projekt/voe3d/cmake/../testing/include/testing/test.h:96:17: note: expanded from macro 'VOE_TEST_CHECK'
             96 |         voe_test_check((expr) ? 1 : 0, __FILE__, __LINE__, #expr)
                |                        ^~~~~~
          /home/ptcsoderlund/Projekt/voe3d/3d/tests/model_levels.c:126:2: note: '?' condition is true
          /home/ptcsoderlund/Projekt/voe3d/cmake/../testing/include/testing/test.h:96:17: note: expanded from macro 'VOE_TEST_CHECK'
             96 |         voe_test_check((expr) ? 1 : 0, __FILE__, __LINE__, #expr)
                |                        ^
          /home/ptcsoderlund/Projekt/voe3d/3d/tests/model_levels.c:129:36: note: The left operand of '==' is a garbage value
            129 |         VOE_TEST_CHECK(medium.level_count == 2 && levels.level_count == 2);
                |                        ~~~~~~~~~~~~~~~~~~ ^
          /home/ptcsoderlund/Projekt/voe3d/cmake/../testing/include/testing/test.h:96:18: note: expanded from macro 'VOE_TEST_CHECK'
             96 |         voe_test_check((expr) ? 1 : 0, __FILE__, __LINE__, #expr)
                |                         ^~~~
          2 warnings generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 6
