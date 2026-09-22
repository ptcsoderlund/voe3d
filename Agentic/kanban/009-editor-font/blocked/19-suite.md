# 19 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: cmake -P check.cmake
            CMakeLists.txt:2 (project)
          
          
          CMake Error at /usr/share/cmake-4.2/Modules/CMakeTestCCompiler.cmake:80 (configure_file):
            Operation not permitted
          Call Stack (most recent call first):
            CMakeLists.txt:2 (project)
          
          
          CMake Error at /usr/share/cmake-4.2/Modules/Internal/CMakeInspectCLinker.cmake:5 (configure_file):
            Operation not permitted
          Call Stack (most recent call first):
            CMakeLists.txt:2 (project)
          
          
          
    CMake Error at check.cmake:70 (message):
      check failed at: standalone 3d
    Call Stack (most recent call first):
      check.cmake:329 (step_fail)
FINDINGS: 1
