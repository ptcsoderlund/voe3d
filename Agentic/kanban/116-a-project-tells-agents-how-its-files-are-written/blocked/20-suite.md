# 20 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
                |                        ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          /mnt/dev/GitRepos/Github/voe3d/cmake/../testing/include/testing/test.h:96:18: note: expanded from macro 'VOE_TEST_CHECK'
             96 |         voe_test_check((expr) ? 1 : 0, __FILE__, __LINE__, #expr)
                |                         ^~~~
          /mnt/dev/GitRepos/Github/voe3d/assets/tests/landscape_keys.c:88:2: note: '?' condition is true
             88 |         VOE_TEST_CHECK(!grid_reads(arena, size->greatest + 1.0, 4, &error));
                |         ^
          /mnt/dev/GitRepos/Github/voe3d/cmake/../testing/include/testing/test.h:96:17: note: expanded from macro 'VOE_TEST_CHECK'
             96 |         voe_test_check((expr) ? 1 : 0, __FILE__, __LINE__, #expr)
                |                        ^
          /mnt/dev/GitRepos/Github/voe3d/assets/tests/landscape_keys.c:92:17: note: Access to field 'greatest' results in a dereference of a null pointer (loaded from variable 'cells')
             92 |                                   (uint32_t)cells->greatest, NULL));
                |                                             ^~~~~
          5 warnings generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 1
