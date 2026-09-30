# 01 — audio may name scene, ecs and math
folder: cmake
after: none
decisions: 0168, 0304

## Change
0304 point 1. In `cmake/voe.cmake`, `voe_allowed_deps`: the `audio` row becomes
`scene ecs math platform assets base`. Its comment gains one point: the sound component and
its system live here beside the mixer, as the emitter lives in `3d`, and read the transform
and camera through `scene` (0304). Nothing else in the file changes; no other row names
`audio` newly.

## Done when
`cmake --preset debug` exits 0, and
`grep -A8 'folder STREQUAL "audio"' cmake/voe.cmake | grep -q 'set(deps scene ecs math platform assets base)'`
exits 0.
