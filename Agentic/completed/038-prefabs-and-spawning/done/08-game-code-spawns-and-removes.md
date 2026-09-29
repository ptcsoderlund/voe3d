# 08 — Game code spawns a prefab by name and removes anything
folder: game
decisions: 0168, 0283, 0242, 0256

## Change
Needs card 02. 0283 point 10.

- `game/include/game/prefabs.h` (new): `VOE_GAME_PREFAB_ENTITIES` 32;
  `typedef bool voe_game_prefab_build(voe_ecs_world *, const voe_ecs_entity *, voe_math_double3,
  voe_math_quat);` a `voe_game_prefab` {`const char *name`, `uint32_t entities`,
  `voe_game_prefab_build *build`}; a `voe_game_prefabs` {`const voe_game_prefab *prefabs`,
  `uint32_t count`}; `extern const voe_game_prefabs voe_game_prefabs_cooked;`. It includes
  `game/scene.h`, so it is the one include a cooked `prefabs.c` needs. Header points: defined by
  the game tree's cooked `prefabs.c`, as `game/scene.h`'s function is by `scene.c`; a name is the
  path under `Assets/` less `.prefab`; only `run.c` names the table.
- `game/include/game/project.h`: `voe_game_project_step` gains `const voe_game_prefabs
  *prefabs` (NULL: nothing to spawn). `[[nodiscard]] bool voe_game_project_spawn(const
  voe_game_project_step *step, const char *name, voe_math_double3 position, voe_math_quat
  rotation, voe_ecs_entity *out_root);` finds `name`, creates its entity count, calls its build;
  on false destroys what it created; an unknown name or no table is one stderr line and false.
  `[[nodiscard]] bool voe_game_project_remove(const voe_game_project_step *step, voe_ecs_entity
  entity);` queues destroying the entity and its tree (`voe_scene_parent_tree`); false when the
  queue is full. Header points: both land at the step's structural apply; why they live in
  `project.c` (0283 point 10: the editor links it, so a loaded library binds).
- `game/src/project.c`: the two definitions.
- `game/include/game/steps.h`, `game/src/steps.c`: `voe_game_steps_run` gains `const
  voe_game_prefabs *prefabs` after `audio`, put in every step it hands out.
- `game/src/run.c`: passes NULL for now (card 20 passes the cooked table).
- `game/tests/steps.c`: the new argument. `game/tests/project.c`: a hand-written two-entity build
  function (root transform at the position, a child with a transform and a parent row naming
  `entities[0]`) in a table; spawn "thing" at (1, 2, 3), apply: the root is there, the child
  under it; unknown name false; remove, apply: both gone; a thousand spawn/remove rounds end
  with the world's entity count back where it began.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: entries.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_game $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_game_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^game/"` exits 0, and `cmake --build --preset debug --target voe_editor && nm -D
build/debug/editor/voe_editor | grep -q " T voe_game_project_spawn"` exits 0.
