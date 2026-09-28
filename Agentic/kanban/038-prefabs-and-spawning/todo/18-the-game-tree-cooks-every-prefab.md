# 18 — The game tree cooks every prefab into prefabs.c
folder: editor
decisions: 0168, 0283, 0235, 0237, 0242

## Change
Needs cards 07 and 08. 0283 point 9.

- `editor/src/project.h`, `editor/src/project.c`: the world is made in one place (the header
  says so); declare that place as `voe_ecs_world *voe_editor_project_world_new(const
  voe_editor_project *project, voe_base_arena *arena);` — game/world.h's types, then the
  project code's own when it has code — if it is not already public, and use it where the file
  made its worlds.
- `editor/src/game_tree.h`, `editor/src/game_tree.c`: `voe_editor_game_tree_write` writes a
  fourth file, `Build/game/prefabs.c`, only when missing or its bytes differ, as the other three:
  - every file under `<folder>/Assets/` whose name ends `.prefab`, in any case, found with
    `platform/folder.h` by an explicit stack of folders (no recursion; a named depth limit),
    hidden entries skipped, taken in byte order of the path;
  - each read, then read into a fresh world from `voe_editor_project_world_new` in rewound
    scratch with `voe_authoring_scene_read`, and cooked with `voe_authoring_prefab_cook`
    (authoring/prefab.h) as `prefab_<n>`; more than `VOE_GAME_PREFAB_ENTITIES` (game/prefabs.h)
    entities, a read or a cook refused: false, why naming the prefab's path;
  - the file: `#include <game/prefabs.h>`, then every `.h` in `Code/` sorted by name as scene.c
    includes them, the functions, then `voe_game_prefabs_cooked` with one entry per prefab —
    its path under `Assets/` less `.prefab`, its entity count, its function — or a count of 0
    with no entries.
  - The header: the layout paragraph lists four files; a paragraph on prefabs.c (0283 point 9).
- `editor/src/src.md`: the changed entries.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0. The human's:
press Play in a copy of `examples/tank_game` after card 13's make; `Build/game/prefabs.c` names
`tank_body` with two entities. The game itself links once cards 19 and 20 are in.
