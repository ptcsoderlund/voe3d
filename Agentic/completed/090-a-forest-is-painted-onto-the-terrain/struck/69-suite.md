# 69 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
                |                                       ^
          /home/ptcsoderlund/Projekt/voe3d/3d/src/model_levels.c:189:6: note: Assuming 'made' is > field 'level_count'
            189 |         if (levels->level_count < made) {
                |             ^~~~~~~~~~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/3d/src/model_levels.c:189:2: note: Taking true branch
            189 |         if (levels->level_count < made) {
                |         ^
          /home/ptcsoderlund/Projekt/voe3d/3d/src/model_levels.c:190:3: note: 'cut' initialized to 0
            190 |                 const uint32_t cut = levels->level_count;
                |                 ^~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/3d/src/model_levels.c:196:3: note: Passed-by-value struct argument contains uninitialized data (e.g., field: 'block')
            196 |                 voe_base_arena_rewind(arena, marks[cut]);
                |                 ^                            ~~~~~~~~~~
          1 warning generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 1
