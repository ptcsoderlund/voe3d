# game

The public headers, one entry each.

- `world.h` — the eight component types a project's world registers, and the room for each.
- `scene.h` — the include the cook is handed: `voe_game_scene_build`, defined by a project's cooked `scene.c`.
- `frame.h` — one frame: the systems, then one window pass through the scene camera; the device capacities it needs.
- `run.h` — the whole run at 1280×720: window, world, cooked scene, frames until the window closes.
