# 11 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING editor/src/src.md: entry `assets_panel.h` is 392 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/interface.c: header comment is 62 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING platform/include/platform/platform.md: does not list `trash.h`
FINDING platform/src/src.md: entry `file_wayland.c` is 335 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING platform/tests/tests.md: entry `file.c` is 363 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING check: failed: cmake -P check.cmake
                |                 ^
          /home/ptcsoderlund/Projekt/voe3d/platform/tests/file.c:199:21: note: Returning from 'read_back'
            199 |         VOE_TEST_CHECK_INT(read_back(MOVE_FOLDER_TO_INNER, got, sizeof got), 1);
                |         ~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/cmake/../testing/include/testing/test.h:99:22: note: expanded from macro 'VOE_TEST_CHECK_INT'
             99 |         voe_test_check_int((actual), (expected), __FILE__, __LINE__, \
                |                             ^~~~~~
          /home/ptcsoderlund/Projekt/voe3d/platform/tests/file.c:200:2: note: 1st function call argument is an uninitialized value
            200 |         VOE_TEST_CHECK_INT(got[0], bytes[0]);
                |         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/cmake/../testing/include/testing/test.h:99:2: note: expanded from macro 'VOE_TEST_CHECK_INT'
             99 |         voe_test_check_int((actual), (expected), __FILE__, __LINE__, \
                |         ^                  ~~~~~~~~
          1 warning generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 6
