# tests

`game`'s own tests: plain C programs, zero for pass, found by the build.

- `world.c` — each of the eight keys resolves on a fresh world, to eight different types.
- `frame.c` — two frames of a camera, a light and a cube on a headless device, captured to a PNG, and again with no light (unshaded, no assert). Skips without a graphics card.
