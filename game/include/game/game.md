# game

The public headers, one entry each.

- `world.h` — the eleven component types a project's world registers, the room for each, and room for a project's own.
- `scene.h` — the include the cook is handed: `voe_game_scene_build`, defined by a project's cooked `scene.c`.
- `frame.h` — the world step (structural queue, project replaces, every owning system) and one frame: that step, then one window pass a lag behind; the device capacities it needs.
- `steps.h` — the fixed steps: elapsed time banked, up to four steps of 1/60 s a frame, the project's systems in two slots, before and after the bodies' move, and the lag the draw sits behind.
- `project.h` — the seam a project's code is written against: its types registered through game, and the three entry points it defines.
- `run.h` — the whole run at 1280×720: window, world, the project's types, cooked scene, the project's systems and a frame until the window closes.
