# 61 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/include/3d/3d.md: entry `models.h` is 335 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/src/src.md: entry `models.c` is 354 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `shape_geometry.c` is 368 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `models.c` is 335 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `models_levels.c` is 334 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `models_material.c` is 310 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING assets/tests/tests.md: entry `model.c` is 322 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING base/tests/tests.md: entry `report.c` is 336 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `models.c` is 306 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/src/src.md: entry `models_materials.c` is 336 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
                |                     ^~~~
          /home/ptcsoderlund/Projekt/voe3d/game/src/models_batch.c:466:3: note: Taking false branch
            466 |                 if (file == NULL)
                |                 ^
          /home/ptcsoderlund/Projekt/voe3d/game/src/models_batch.c:470:24: note: Assuming 'k' is < field 'picture_count'
            470 |                 for (uint32_t k = 0; k < file->picture_count; k++)
                |                                      ^~~~~~~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/game/src/models_batch.c:470:3: note: Loop condition is true.  Entering loop body
            470 |                 for (uint32_t k = 0; k < file->picture_count; k++)
                |                 ^
          /home/ptcsoderlund/Projekt/voe3d/game/src/models_batch.c:471:26: note: Array access (via field 'pictures') results in a null pointer dereference
            471 |                         batch->pictures[at++] = (struct batch_picture){
                |                                ~~~~~~~~       ^
          1 warning generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 11
