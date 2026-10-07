# 24 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: cmake -P check.cmake
                |                              ^~~~~~~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/3d/src/models.c:97:2: note: Loop condition is false. Execution continues on line 99
             97 |         for (uint32_t i = 0; i < held->shading_count; i++)
                |         ^
          /home/ptcsoderlund/Projekt/voe3d/3d/src/models.c:99:23: note: Assuming 'i' is < field 'texture_count'
             99 |         for (uint32_t i = 0; i < held->texture_count; i++)
                |                              ^~~~~~~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/3d/src/models.c:99:2: note: Loop condition is true.  Entering loop body
             99 |         for (uint32_t i = 0; i < held->texture_count; i++)
                |         ^
          /home/ptcsoderlund/Projekt/voe3d/3d/src/models.c:100:44: note: Array access (via field 'textures') results in a null pointer dereference
            100 |                 (void)voe_render_texture_destroy(device, held->textures[i]);
                |                                                          ^     ~~~~~~~~
          2 warnings generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 1
