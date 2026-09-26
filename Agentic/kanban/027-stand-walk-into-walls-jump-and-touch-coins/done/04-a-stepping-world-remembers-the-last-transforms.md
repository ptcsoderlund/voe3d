# 04 — A stepping world remembers the last transforms, and gives one in between
folder: scene
decisions: 0168, 0254, 0065

## Change
0254 point 3 (ADR-0065's D-083): the previous state belongs to the transform module.

- `scene/include/scene/transform_system.h` — three calls:
  - `voe_scene_transform_previous_register(world, capacity)` — a runtime-only table (its own
    key, `&voe_ecs_runtime_only`, no replace, no menu) of `voe_scene_transform`, for a world that
    steps; a world that does not never calls it.
  - `voe_scene_transform_remember(world)` — every transform row copied into the previous table,
    adding a row for an entity that has none; asserts the table is registered.
  - `voe_scene_transform_between(world, entity, float lag)` → `voe_scene_transform`: the
    transform `lag` of a step back from the current one (0 now, 1 the previous): position blended
    in double, rotation the shortest-path blend normalised, scale straight. The current row when
    lag is 0, when the table is not registered, or the entity has no previous row. Asserts the
    entity has a transform and 0 ≤ lag ≤ 1.
  Header points: why the previous state is here and not in physics (ADR-0065 point 4, ADR-0011);
  why opt-in (the editor steps nothing, 0254); why a lag and not an alpha (nought means now).
- `scene/src/transform_system.c` (or a new `scene/src/transform_previous.c` if the file would pass
  ~400 lines; list it on `scene/src/src.md`) — the three.
- `scene/tests/transform.c` — remember, move, between at 0, 0.5 and 1 (position at 100 km exact
  to the millimetre, a 90° turn halved, scale halfway); unregistered or never remembered gives the
  current row; a runtime-only type (`voe_ecs_component_runtime_only` true).
- `scene/scene.md` — the transform system entry names the previous table and the blend.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder scene` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^scene/transform$'` passes.
