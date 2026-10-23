# src

`game`'s implementation, one file per public header except `scene.h`,
`prefabs.h`, `landscapes.h` and `materials.h`, which a project's cooked C
defines, and `models.h`, which has two and an internal batch.

- `world.c` — the twenty-seven registrations and eighteen intent queues, the emitter, its particles, the sound, its voice row, the water, its waves, the point light, the light blocker, the sky, the haze and the picture among them, their capacities and the room for a project's types.
- `frame.c` — the world step's drains in order and the frame: that step, the blockers and lights, the foliage gathered after them, the shadow passes, then the window pass lit by them, its counts handed back, with the interface over the world.
- `steps.c` — one fixed step's calls in order (systems, world step, move, transforms, after-the-move systems, world step again, emitters, point lights, waters, sounds with the window's aspect), the mixer in each step, and the bank.
- `project.c` — the 32 replace keys, a project type's registration and the drain of its replaces; a prefab spawned by name and a tree removed, both queued.
- `interface.c` — the interface made once, the surface's millimetres, and a frame begun with the pointer divided into them and the asks put in.
- `starting.c` — the window polled without the app's pace, the line laid out on a ground panel, plain or over the splash's edge colour and centred picture, and drawn in one element-only pass; the wait's worker thread, frame loop and timed "Loaded in" hold; the shaders step.
- `progress.c` — the progress record's atomic stores and loads, its line and the "Loaded in" line.
- `run.c` — the run's steps in the handed window and its threads, the view's counts said once a second under `VOE_GAME_COUNTS=1`, and the only file naming the cooked scene, prefabs, landscapes, materials and parts, the materials and parts loaded after each models update.
- `splashscreen.png` — the engine's splash, read by the editor from the source and put beside a game's program by `cmake/game.cmake` when the project has none (0356).
- `models.c` — an update's, watch's or reload's paths loaded one file at a time as batches of one, a cooked landscape with its kinds, their base sizes and copies and each landscape's kind paths, each failure counted.
- `models_batch.h` — internal: a batch of files through timed phases in order on the caller, taking and writing cooked copies as its mode says, then put; the phases' times and their line; the levels file helpers `models.c` shares.
- `models_batch.c` — each phase's tasks in order under the file's path, a `.glb`'s cooked copy taken or its levels file read or made and written, the copy cooked, the put keeping a refused file failed, and the times' line.
- `models_cooked.h` — internal: who takes and writes a `.glb`'s or a material map's cooked copy (the modes), their paths, a model's key, and the folders and `.parts` helpers the batch shares.
- `models_cooked.c` — a model's copy in `Cache/models/` and a map's in `Cache/pictures/`, each read and checked against its key, or mipped and written with its folders made; a model's key from the stamps; the editor's stale count from copies' heads and its device-free cook of them.
- `models_materials.c` — each loaded model given its parts; each material a shape, a part's `wears` or a landscape layer names that the store lacks gathered from the table or kept failed, each map from its cooked copy or read, decoded and cooked, uploaded one at a time, and the times line.
