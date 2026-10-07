# 16 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: cmake -P check.cmake
                |             ^~~~~~~~~~~~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/platform/src/trash_wayland.c:248:2: note: Taking true branch
            248 |         if (rename(path, file_path) != 0) {
                |         ^
          /home/ptcsoderlund/Projekt/voe3d/platform/src/trash_wayland.c:251:9: note: Assuming that 'unlink' is successful; 'errno' becomes undefined after the call
            251 |                 (void)unlink(info_path);
                |                       ^~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/platform/src/trash_wayland.c:252:10: note: An undefined value may be read from 'errno'
            252 |                 return errno == EXDEV ? VOE_BASE_ERROR_UNSUPPORTED
                |                        ^~~~~
          /usr/include/errno.h:38:16: note: expanded from macro 'errno'
             38 | # define errno (*__errno_location ())
                |                ^~~~~~~~~~~~~~~~~~~~~~
          1 warning generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 1
