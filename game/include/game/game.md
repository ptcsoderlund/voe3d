# game

The public headers, one entry each.

- `world.h` — the thirty-one component types a project's world registers, the sound, the water, the point light, the light blocker, the one sky, haze, picture and clouds with their drift, and the fog volumes with theirs among them, the room for each, and room for a project's own.
- `scene.h` — the include the cook is handed: `voe_game_scene_build`, defined by a project's cooked `scene.c`.
- `frame.h` — the world step and one frame of it: the step, the sky, haze and picture rows' drains among it, the window pass with its sky, haze and picture and the interface over it, its drawn and skipped counts handed back on request, with the device capacities it needs.
- `steps.h` — the fixed steps: elapsed time banked, up to four steps of 1/60 s a frame, the project's systems in two slots, before and after the bodies' move, the clouds' and fog's drift, the lag the draw sits behind, and the mixer put in every step and playing the sounds last.
- `project.h` — the seam a project's code is written against: its types registered through game, the step with the mixer and prefab table, spawning a prefab by name and removing a tree, the interface's asks to pause and restart, and the four entry points.
- `prefabs.h` — the cooked prefab table a game spawns from, defined by a project's cooked `prefabs.c`, and the one include that file needs.
- `interface.h` — the project's interface: font, theme and ui context, the 135 mm surface, and one frame begun with the pointer and the run's asks and handed to the project.
- `starting.h` — the starting frame, plain (one line centred on the theme's ground) or a splash picture with the line beneath; the wait that shows it while a worker runs a start's work; the shaders step with its cache.
- `progress.h` — the record a splash wait's worker reports its phase and count into and the main thread reads its line from, with the stop flag, all atomic; and the "Loaded in N.N s" line a wait ends on.
- `run.h` — the game's whole run in the window it is handed, its start on a worker behind the splash, until the window closes or the interface ends it, saying its counts when `VOE_GAME_COUNTS` asks.
- `models.h` — the model files a world names read from a folder into a store, and re-read when their stamp changes, with each landscape's foliage kinds and the cooked landscape, parts and material tables loaded into it; shared by the game and the editor.
- `landscapes.h` — the cooked landscape table a game loads into its model store, each entry with its layer paths and paint and its foliage kinds, copies, kinds' looks and layers' growth, defined by a project's cooked `landscapes.c`.
- `compressing.h` — the queue of up to sixteen material maps compressed between frames on the owner's thread, a budget a step, each written as its copy when its blocks are made (0477).
- `materials.h` — the material table of path and values and the parts table of `.glb` path and `.parts` values a game loads into its model store, both defined by a project's cooked `materials.c`; the editor builds its own.
