# tests

`game`'s own tests: plain C programs, zero for pass, found by the build.

- `world.c` — each of the twenty-two public keys, the point light's and the light blocker's among them, resolves on a fresh world, to twenty-two different types, and the world counts twenty-three; its keys come through `game/scene.h`.
- `project.c` — a project type's menu, default, replaces on a game world, its need named by a call and refused when wrong, and a two-entity prefab spawned, refused by unknown name, removed and repeated a thousand times without a leak.
- `steps.c` — the fixed steps and their lag on a game world, and what each step moves: bodies, followers, emitters, sounds, water and a point light's replace, as a fade sends it.
- `frame.c` — headless game frames of a camera, lights and shapes, with and without a light, with sun, moon and lamp shadows, light and Direct blockers read back, an interface and a missing model, each true. Skips without a graphics card.
- `interface.c` — a 1280×720 surface is 240×135 mm; a run hands back a stand-in interface's answer and the asks it set, and a panel with a label and a button leaves element records. Skips without a graphics card.
- `starting.c` — the starting frame, plain and with a splash in a wide and a tall window, read back, the wait around the shaders step leaving it prepared with its cache written and a refusing work's wait false, and the progress line. Skips without a graphics card.
- `models.c` — the model loader on real files: a hand-built `.glb` loaded, missing and watched, emitter pictures, the water record, an update's progress and stop, and a landscape table; skips without a graphics card.
