# game

The public headers, one entry each.

- `world.h` — the twenty-seven component types a project's world registers, the sound, its voice row, the water, its waves, the point light, the light blocker, the one sky, haze and picture among them, the room for each, and room for a project's own.
- `scene.h` — the include the cook is handed: `voe_game_scene_build`, defined by a project's cooked `scene.c`.
- `frame.h` — the world step and one frame of it: the step, the sky, haze and picture rows' drains among it, the window pass with its sky, haze and picture and the interface over it, with the device capacities it needs.
- `steps.h` — the fixed steps: elapsed time banked, up to four steps of 1/60 s a frame, the project's systems in two slots, before and after the bodies' move, the lag the draw sits behind, and the mixer put in every step and playing the sounds last.
- `project.h` — the seam a project's code is written against: its types registered through game, the step with the mixer and prefab table, spawning a prefab by name and removing a tree, the interface's asks to pause and restart, and the four entry points.
- `prefabs.h` — the cooked prefab table a game spawns from, defined by a project's cooked `prefabs.c`, and the one include that file needs.
- `interface.h` — the project's interface: font, theme and ui context, the 135 mm surface, and one frame begun with the pointer and the run's asks and handed to the project.
- `starting.h` — the starting frame, plain (one line centred on the theme's ground) or a splash picture with the line beneath; the wait that shows it while a worker runs a start's work; the shaders step with its cache.
- `progress.h` — the record a splash wait's worker reports its phase and count into and the main thread reads its line from, with the stop flag; all atomic.
- `run.h` — the game's whole run in the window it is handed, its start on a worker behind the splash, until the window closes or the interface ends it.
- `models.h` — the model files a world names read from a folder into a store, and re-read when their stamp changes, with each landscape's foliage kinds and the cooked landscape, parts and material tables loaded into it; shared by the game and the editor.
- `models_loader.h` — the editor's model loader beside the frames: what an update and the watch load, read to baked on a thread of its own over the store's pool and put one file a frame.
- `landscapes.h` — the cooked landscape table a game loads into its model store, each entry with its layer paths and paint and its foliage kinds and copies, defined by a project's cooked `landscapes.c`.
- `materials.h` — the material table of path and values and the parts table of `.glb` path and `.parts` values a game loads into its model store, both defined by a project's cooked `materials.c`; the editor builds its own.
