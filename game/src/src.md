# src

`game`'s implementation, one file per public header except `scene.h` and
`prefabs.h`, which a project's cooked `scene.c` and `prefabs.c` define.

- `world.c` — the twenty-three registrations and fifteen intent queues, the emitter, its particles, the sound, its voice row, the water, its waves, the point light and the light blocker among them, their capacities and the room for a project's types.
- `frame.c` — the world step's drains in order, the sounds last with no mixer, and the frame: that step, the light blockers, the point lights, the shadow passes, the sun's and the lamps', then the window pass lit by them with the interface's records over the world.
- `steps.c` — one fixed step's calls in order (systems, world step, move, transforms, after-the-move systems, world step again, emitters, point lights, waters, sounds with the window's aspect), the mixer in each step, and the bank.
- `project.c` — the 32 replace keys, a project type's registration and the drain of its replaces; a prefab spawned by name and a tree removed, both queued.
- `interface.c` — the interface made once, the surface's millimetres, and a frame begun with the pointer divided into them and the asks put in.
- `starting.c` — the window polled without the app's pace, the line laid out on a ground panel, plain or over the splash's edge colour and centred picture, and drawn in one element-only pass; the prepare loop around it.
- `run.c` — the run's steps in the handed window; the only file naming `voe_game_scene_build`, `voe_game_prefabs_cooked` and the project's entry points.
- `models.c` — a path joined onto the folder, stamped, read into rewound scratch and loaded, the dot and the water record when the world needs them; each failure a stderr line and counted.
