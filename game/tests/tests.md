# tests

`game`'s own tests: plain C programs, zero for pass, found by the build.

- `world.c` — each of the sixteen public keys, the parent's, the two prefab keys, the emitter's and the particles' among them, resolves on a fresh world, to sixteen different types, and the world counts seventeen; its keys come through `game/scene.h`.
- `project.c` — a project type's menu, default and replaces on a game world: applied whole, dropped for a destroyed entity, a 241-byte row refused; a two-entity prefab spawned under its root, an unknown name refused, removed whole, a thousand rounds leaking nothing.
- `steps.c` — the fixed steps and their lag, a body landing on a box floor, a follower at the body's position by the end of the step, and an emitter holding live particles after 60 steps.
- `frame.c` — two headless frames of a camera, a light and a cube, with and without the light; a capsule casting onto the cube, every shadow pass fitting, lit or not; a thing wearing a path an empty model store lacks, drawn with that store. Skips without a graphics card.
- `interface.c` — a 1280×720 surface is 240×135 mm; a run hands back a stand-in interface's answer, and a panel with a label and a button leaves element records. Skips without a graphics card.
- `models.c` — the model loader on real files: a hand-built `.glb` loaded, missing and watched, and an emitter's pictures loaded or missing. Skips without a graphics card.
