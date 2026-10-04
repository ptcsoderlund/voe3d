# 11 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/tests/tests.md: entry `light_blockers.c` is 330 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `view_passes.c` is 314 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/src/descriptors.c: header comment is 63 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING render/tests/tests.md: entry `bounce_probes.c` is 319 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING scene/include/scene/scene.md: does not list `light_blocker_component.h`
FINDING scene/include/scene/scene.md: does not list `light_blocker_system.h`
FINDING check: failed: cmake -P check.cmake
                |                        ^
          /home/ptcsoderlund/Projekt/voe3d/render/tests/blocked_bounce.c:431:2: note: '?' condition is true
            431 |         VOE_TEST_CHECK(gap_at(s, &out, &out_none, PATCH) <= TOLERANCE);
                |         ^
          /home/ptcsoderlund/Projekt/voe3d/cmake/../testing/include/testing/test.h:96:17: note: expanded from macro 'VOE_TEST_CHECK'
             96 |         voe_test_check((expr) ? 1 : 0, __FILE__, __LINE__, #expr)
                |                        ^
          /home/ptcsoderlund/Projekt/voe3d/render/tests/blocked_bounce.c:431:2: note: Address of stack memory associated with local variable 'house' is still referred to by the caller variable 'scene' upon returning to the caller.  This will be a dangling reference
            397 |         VOE_TEST_CHECK(gap_at(s, &out, &out_none, PATCH) <= TOLERANCE);
                |         ^
          /home/ptcsoderlund/Projekt/voe3d/cmake/../testing/include/testing/test.h:96:2: note: expanded from macro 'VOE_TEST_CHECK'
             96 |         voe_test_check((expr) ? 1 : 0, __FILE__, __LINE__, #expr)
                |         ^
          1 warning generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 7
