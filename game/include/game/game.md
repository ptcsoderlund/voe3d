# game

The public headers, one entry each.

- `world.h` — the eight component types a project's world registers, the room for each, and room for a project's own.
- `scene.h` — the include the cook is handed: `voe_game_scene_build`, defined by a project's cooked `scene.c`.
- `frame.h` — one frame: the structural queue and project replaces, the systems, then one window pass through the scene camera; the device capacities it needs.
- `project.h` — the seam a project's code is written against: its types registered through game, and the two entry points it defines.
- `run.h` — the whole run at 1280×720: window, world, the project's types, cooked scene, the project's systems and a frame until the window closes.
