# tests

`game`'s own tests: plain C programs, zero for pass, found by the build.

- `world.c` — each of the sixteen public keys, the parent's, the two prefab keys, the emitter's and the particles' among them, resolves on a fresh world, to sixteen different types, and the world counts seventeen; its keys come through `game/scene.h`.
- `project.c` — a project type's menu, default and replaces on a game world: applied whole, dropped for a destroyed entity, a 241-byte row refused; a two-entity prefab spawned under its root, an unknown name refused, removed whole, a thousand rounds leaking nothing.
- `steps.c` — steps counted and the lag for one, two and a half and sixty steps' time, a body pulled onto a box floor standing after 60 steps, and a follower set in the after-the-move slot at the body's position by the end of the step, and an emitter of rate 60 holding live particles after 60 steps.
- `frame.c` — two headless frames of a camera, a light and a cube, with and without the light; a capsule casting onto the cube, every shadow pass fitting, lit or not; a thing wearing a path an empty model store lacks, drawn with that store. Skips without a graphics card.
- `interface.c` — a 1280×720 surface is 240×135 mm; a run hands back a stand-in interface's answer, and a panel with a label and a button leaves element records. Skips without a graphics card.
- `models.c` — a hand-built one-triangle `.glb` in a scratch folder loaded by a row, a missing file failing by name, and a watch failing on text and reloading when valid again, with no dot; an emitter's PNG loaded as a picture with the dot at "", and a missing one failing by name. Skips without a graphics card.
