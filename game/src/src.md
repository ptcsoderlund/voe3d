# src

`game`'s implementation, one file per public header except `scene.h`,
`prefabs.h`, `landscapes.h` and `materials.h`, which a project's cooked C
defines, and `models.h`, which has two and an internal batch.

- `world.c` — the twenty-seven registrations and eighteen intent queues, the emitter, its particles, the sound, its voice row, the water, its waves, the point light, the light blocker, the sky, the haze and the picture among them, their capacities and the room for a project's types.
- `frame.c` — the world step's drains in order, the sky's, the haze's and the picture's after the light's, and the frame: that step, the light blockers, the point lights, the lights after the first, the shadow passes, then the window pass lit by them with the interface over the world.
- `steps.c` — one fixed step's calls in order (systems, world step, move, transforms, after-the-move systems, world step again, emitters, point lights, waters, sounds with the window's aspect), the mixer in each step, and the bank.
- `project.c` — the 32 replace keys, a project type's registration and the drain of its replaces; a prefab spawned by name and a tree removed, both queued.
- `interface.c` — the interface made once, the surface's millimetres, and a frame begun with the pointer divided into them and the asks put in.
- `starting.c` — the window polled without the app's pace, the line laid out on a ground panel, plain or over the splash's edge colour and centred picture, and drawn in one element-only pass; the wait's worker thread and frame loop; the shaders step.
- `progress.c` — the progress record's atomic stores and loads and its line.
- `run.c` — the run's steps in the handed window and its threads, and the only file naming the cooked scene, prefabs, landscapes, materials and parts, the materials and parts loaded after each models update.
- `splashscreen.png` — the engine's splash, read by the editor from the source and put beside a game's program by `cmake/game.cmake` when the project has none (0356).
- `models.c` — an update's, watch's or reload's paths loaded one file at a time as batches of one, a cooked landscape with its kinds and copies and each landscape's kind paths, each failure counted.
- `models_batch.h` — internal: a batch of files through timed phases in order on the caller, then put; the phases' times and their line; the levels file helpers `models.c` shares.
- `models_batch.c` — each phase's tasks in order under the file's path, a `.glb`'s levels file read or made and written, the put keeping a refused file failed, and the times' line.
- `models_materials.c` — each loaded model given its parts; each material a shape, a part's `wears` or a landscape layer names that the store lacks gathered from the table or kept failed, its maps read, decoded and uploaded one material at a time on the caller, and the materials' times line.
