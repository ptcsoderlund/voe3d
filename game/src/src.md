# src

`game`'s implementation, one file per public header except `scene.h` and
`prefabs.h`, which a project's cooked `scene.c` and `prefabs.c` define.

- `world.c` — the nineteen registrations, the emitter, its particles, the sound and its voice row among them, their capacities and the room for a project's types.
- `frame.c` — the world step's drains in order, the sounds last with no mixer, and the frame: that step, the sun's shadow passes, then the window pass with the interface's records over the world.
- `steps.c` — one fixed step's calls in order (systems, world step, move, transforms, after-the-move systems, world step again, emitters, sounds with the window's aspect), the mixer in each step, and the bank the steps are drawn from.
- `project.c` — the 32 replace keys, a project type's registration and the drain of its replaces; a prefab spawned by name and a tree removed, both queued.
- `interface.c` — the interface made once, the surface's millimetres, and a frame begun with the pointer divided into them.
- `run.c` — the run's steps in the handed window and their refusals, the interface each frame and its end of the run, the mixer and a sound device pumped or silent; the only file naming `voe_game_scene_build`, `voe_game_prefabs_cooked` and the project's entry points.
- `models.c` — a path joined onto the folder, stamped, read into rewound scratch and loaded; each failure a stderr line and counted.
