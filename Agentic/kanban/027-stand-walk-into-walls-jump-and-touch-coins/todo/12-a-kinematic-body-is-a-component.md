# 12 — A kinematic body is a component
folder: physics
decisions: 0168, 0249, 0253

## Change
0249 layer 2's data, 0253 point 4. The move is card 13.

- `physics/include/physics/body_component.h` (new) — `voe_physics_body` described:
  `step_height` FLOAT32 (metres), `slope_limit` FLOAT32 (radians from level), `velocity` FLOAT3
  read-only, `on_floor` BOOL read-only. Key `voe_physics_body_key` (`extern VOE_BASE_IMPORTED`);
  `_get`, `_count`, `_rows`, `_entities`. Header points: `velocity` is the wanted velocity when a
  project submits it and what the body really moved after a move, so a project reads a landing
  or a ceiling from it; gravity and jumping are the project's (0249); read-only is the
  Inspector's note, the project still writes the whole row through the intent.
- `physics/include/physics/body_system.h` (new) — `voe_physics_body_register(world, capacity)`
  (table, description, whole-row replace intent, default step 0.3, slope 0.8, still, not on the
  floor, needs a collider, menu "Physics / Kinematic Body"); `_add`; `voe_physics_body_intent`,
  `_submit`; `voe_physics_body_system_run(world)` drains: a step height below 0 or not finite, a
  slope limit outside [0, π/2] or a velocity not finite keeps the row, reported as the collider's
  drain reports. Header point: the drain runs every frame in the editor and the game; the move
  (card 13) only in the game's steps.
- `physics/src/body_component.c`, `physics/src/body_system.c` (new); `physics/src/src.md`.
- `physics/tests/body.c` (new) — register after colliders, a row replaced whole, each refusal
  kept, the default row, needs the collider type. `physics/tests/tests.md`.
- `physics/physics.md` — the two entries.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder physics` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^physics/body$'` passes.
