# game

The public headers, one entry each.

- `world.h` — the twenty-three component types a project's world registers, the sound, its voice row, the water, its waves, the point light and the light blocker among them, the room for each, and room for a project's own.
- `scene.h` — the include the cook is handed: `voe_game_scene_build`, defined by a project's cooked `scene.c`.
- `frame.h` — the world step (structural queue, project replaces, every owning system, the sounds last with no mixer) and one frame: that step, then one window pass a lag behind, kept out of light blockers, with the interface over it; the device capacities it needs.
- `steps.h` — the fixed steps: elapsed time banked, up to four steps of 1/60 s a frame, the project's systems in two slots, before and after the bodies' move, the lag the draw sits behind, and the mixer put in every step and playing the sounds last.
- `project.h` — the seam a project's code is written against: its types registered through game, the step with the mixer and prefab table, spawning a prefab by name and removing a tree, the interface's asks to pause and restart, and the four entry points.
- `prefabs.h` — the cooked prefab table a game spawns from, defined by a project's cooked `prefabs.c`, and the one include that file needs.
- `interface.h` — the project's interface: font, theme and ui context, the 135 mm surface, and one frame begun with the pointer and the run's asks and handed to the project.
- `starting.h` — the starting frame, one line centred on the theme's ground while the device builds its pipelines, and the prepare loop that draws it between steps.
- `run.h` — the game's whole run in the window it is handed, until the window closes or the interface ends it.
- `models.h` — the model files a world names read from a folder into a store, and re-read when their stamp changes; shared by the game and the editor.
