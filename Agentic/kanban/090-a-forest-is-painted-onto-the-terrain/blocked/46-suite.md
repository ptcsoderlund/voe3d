# 46 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
                |             ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/assets/src/model_gltf_meshes.c:462:2: note: Taking false branch
            462 |         if (!mesh_primitives(gltf, mesh, &array, &out->primitive_count))
                |         ^
          /home/ptcsoderlund/Projekt/voe3d/assets/src/model_gltf_meshes.c:465:23: note: Assuming 'p' is < field 'primitive_count'
            465 |         for (uint32_t p = 0; p < out->primitive_count; p++) {
                |                              ^~~~~~~~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/assets/src/model_gltf_meshes.c:465:2: note: Loop condition is true.  Entering loop body
            465 |         for (uint32_t p = 0; p < out->primitive_count; p++) {
                |         ^
          /home/ptcsoderlund/Projekt/voe3d/assets/src/model_gltf_meshes.c:466:24: note: 2nd function call argument is an uninitialized value
            466 |                 uint32_t primitive = voe_assets_gltf_object_at(gltf, array, p);
                |                                      ^                               ~~~~~
          2 warnings generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 1
