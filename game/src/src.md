# src

`game`'s implementation, one file per public header except `scene.h`,
`prefabs.h`, `landscapes.h` and `materials.h`, which a project's cooked C
defines, and `models.h`, which has two and an internal batch.

- `world.c` — the thirty-one registrations and twenty intent queues, the emitter, the sound, its voice row, the water, its waves, the point light, the light blocker, the sky, the haze, the picture, the clouds and the fog volumes among them, their capacities and the room for a project's types.
- `frame.c` — the world step's drains in order and the frame: that step, the blockers and lights, the foliage gathered after them, the shadow passes, then the window pass lit by them, its counts handed back, with the interface over the world.
- `steps.c` — one fixed step's calls in order (systems, world step, move, transforms, after-the-move systems, world step again, emitters, point lights, waters, clouds, fog, sounds with the window's aspect), the mixer in each step, and the bank.
- `project.c` — the 32 replace keys, a project type's registration and the drain of its replaces; a prefab spawned by name and a tree removed, both queued.
- `interface.c` — the interface made once, the surface's millimetres, and a frame begun with the pointer divided into them and the asks put in.
- `starting.c` — the window polled without the app's pace, the line laid out on a ground panel, plain or over the splash's edge colour and centred picture, and drawn in one element-only pass; the wait's worker thread, frame loop and timed "Loaded in" hold; the shaders step.
- `progress.c` — the progress record's atomic stores and loads, its line and the "Loaded in" line.
- `run.c` — the run's steps in the handed window and its threads, the view's counts said once a second under `VOE_GAME_COUNTS=1`, and the only file naming the cooked scene, prefabs, landscapes, materials and parts, the materials and parts loaded after each models update.
- `splashscreen.png` — the engine's splash, read by the editor from the source and put beside a game's program by `cmake/game.cmake` when the project has none (0356).
- `models.c` — an update's, watch's or reload's paths loaded in batches, up to four files under a progress, else one, a cooked landscape with its kinds, their looks, its layers' growth and its copies and each landscape's kind paths, each failure counted.
- `models_batch.h` — internal: a batch of files through timed phases, each up to baking a burst on workers, cooking and the put on the caller, a worker's error carried to it, taking and writing cooked copies as its mode says; the phases' times and their line; the levels file helpers.
- `models_batch.c` — each phase's tasks as one burst under the file's path, a `.glb`'s copy taken or its levels file read or made, a picture only dressed parts use a white texel, pictures mipped when cooked, the copy cooked on the caller, a refused file kept failed, and the times' line.
- `models_cooked.h` — internal: who takes and writes a `.glb`'s or a material map's cooked copy (the modes), their paths, a model's key, and the folders and `.parts` helpers the batch shares.
- `models_cooked.c` — a model's copy in `Cache/models/` and a map's in `Cache/pictures/`, each read and checked against its key, or mipped, compressed on the GPU and written; a model's key from the stamps; the editor's stale count from copies' heads and its cook on the caller's device.
- `compressing.c` — the queue's jobs, each an arena and a stage, the oldest moved one stage a step: read and mipped on a one-worker burst, compressed on the GPU, written on a one-worker burst through `models_cooked.h`.
- `models_materials.c` — each loaded model given its parts; each material a shape, part or layer names the store lacks gathered or kept failed, its maps taken, queued, or read, decoded and mipped in one burst, its error carried, then cooked or queued and uploaded on the caller, and the times line.
