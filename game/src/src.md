# src

`game`'s implementation, one file per public header except `scene.h`,
`prefabs.h`, `landscapes.h` and `materials.h`, which a project's cooked C
defines, and `models.h`, which has two.

- `world.c` — the twenty-seven registrations and eighteen intent queues, the emitter, its particles, the sound, its voice row, the water, its waves, the point light, the light blocker, the sky, the haze and the picture among them, their capacities and the room for a project's types.
- `frame.c` — the world step's drains in order, the sky's, the haze's and the picture's after the light's, and the frame: that step, the light blockers, the point lights, the lights after the first, the shadow passes, then the window pass lit by them with the interface over the world.
- `steps.c` — one fixed step's calls in order (systems, world step, move, transforms, after-the-move systems, world step again, emitters, point lights, waters, sounds with the window's aspect), the mixer in each step, and the bank.
- `project.c` — the 32 replace keys, a project type's registration and the drain of its replaces; a prefab spawned by name and a tree removed, both queued.
- `interface.c` — the interface made once, the surface's millimetres, and a frame begun with the pointer divided into them and the asks put in.
- `starting.c` — the window polled without the app's pace, the line laid out on a ground panel, plain or over the splash's edge colour and centred picture, and drawn in one element-only pass; the wait's worker thread and frame loop; the shaders step.
- `progress.c` — the progress record's atomic stores and loads and its line.
- `run.c` — the run's steps in the handed window and its threads, and the only file naming the cooked scene, prefabs, landscapes, materials and parts, the materials and parts loaded after each models update.
- `splashscreen.png` — the engine's splash, read by the editor from the source and put beside a game's program by `cmake/game.cmake` when the project has none (0356).
- `models.c` — a path joined onto the folder, stamped, read and loaded, a `.glb` with its levels file and a cooked landscape among them, each failure counted.
- `models_materials.c` — each loaded model given its parts from the parts table; a table material's four maps read from the folder into rewound scratch and loaded; each shape's material, part's `wears` and landscape layer the store lacks found in the table or kept failed, each failure counted.
