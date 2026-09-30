# tests

`game`'s own tests: plain C programs, zero for pass, found by the build.

- `world.c` — each of the sixteen public keys, the parent's, the two prefab keys, the emitter's and the particles' among them, resolves on a fresh world, to sixteen different types, and the world counts seventeen; its keys come through `game/scene.h`.
- `project.c` — a project type's menu, default, replaces on a game world, its need named by a call and refused when wrong, and a two-entity prefab spawned, refused by unknown name, removed and repeated a thousand times without a leak.
- `steps.c` — the fixed steps and their lag, a body landing on a box floor, a follower at the body's position by the end of the step, and an emitter holding live particles after 60 steps.
- `frame.c` — two headless frames of a camera, a light and a cube, with and without the light; a capsule casting onto the cube, every shadow pass fitting, lit or not; a thing wearing a path an empty model store lacks, drawn with that store. Skips without a graphics card.
- `interface.c` — a 1280×720 surface is 240×135 mm; a run hands back a stand-in interface's answer, and a panel with a label and a button leaves element records. Skips without a graphics card.
- `models.c` — the model loader on real files: a hand-built `.glb` loaded, missing and watched, and an emitter's pictures loaded or missing. Skips without a graphics card.
