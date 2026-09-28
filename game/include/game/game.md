# game

The public headers, one entry each.

- `world.h` — the fifteen component types a project's world registers, the room for each, and room for a project's own.
- `scene.h` — the include the cook is handed: `voe_game_scene_build`, defined by a project's cooked `scene.c`.
- `frame.h` — the world step (structural queue, project replaces, every owning system) and one frame: that step, then one window pass a lag behind with the interface over it; the device capacities it needs.
- `steps.h` — the fixed steps: elapsed time banked, up to four steps of 1/60 s a frame, the project's systems in two slots, before and after the bodies' move, the lag the draw sits behind, and the mixer put in every step.
- `project.h` — the seam a project's code is written against: its types registered through game, the step with the mixer its systems play sounds through, and the four entry points.
- `interface.h` — the project's interface: font, theme and ui context, the 135 mm surface, and one frame begun with the pointer and handed to the project.
- `run.h` — the whole run at 1280×720: window, world, the project's types, the mixer and sound device, cooked scene, the project's systems, its interface and a frame until the window closes or the interface ends the run.
- `models.h` — the model files a world names read from a folder into a store, and re-read when their stamp changes; shared by the game and the editor.
