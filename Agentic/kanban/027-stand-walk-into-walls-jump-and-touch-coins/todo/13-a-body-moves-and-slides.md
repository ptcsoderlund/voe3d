# 13 — A body moves, slides, climbs a gentle slope and steps up a ledge
folder: physics
decisions: 0168, 0249, 0253, 0250

## Change
0253 point 4. The move reads, then writes only its own rows and submits transform intents
(0249 rule 2).

- `physics/include/physics/body_system.h` — `voe_physics_body_system_move(world, float seconds)`:
  every body with a capsule or sphere collider and a transform moves by its `velocity` × seconds,
  submits `voe_scene_transform_intent` with the new double position (rotation and scale kept),
  and writes its own row: `velocity` what it really moved / seconds, `on_floor`. Header points:
  what one call does, in the order below; that the caller drains the transform queue before the
  next move; triggers and the body's own collider are passed through.
- `physics/src/body_move.c` (new, the move alone; list it on `src/src.md`) — per body:
  1. Substeps so none moves further than a quarter of the radius.
  2. Each substep: move; then up to 4 times: `voe_physics_overlap` at the new place (ignoring
     itself and triggers), push out of the deepest contact. A contact whose normal is within the
     slope limit of up is floor: push straight up by depth / normal.y, set on_floor, drop the
     downward velocity. Any other is wall: its normal flattened to horizontal, push out along it,
     drop the velocity into it (this is the slide, and why a steep ramp is not climbed).
  3. Step up: when a substep on the floor met a wall, try again from `step_height` higher and
     settle down onto a floor; keep it if it lands on floor further along, else keep the first.
  4. Stay on the floor: when it was on the floor and is not rising, a probe down by
     `step_height` that finds floor puts it there (walking down a ramp or off a ledge's lip
     without hopping); otherwise it is off the floor.
- `physics/tests/body.c` — a capsule (1, 2, 1) at 60 steps a second with the test's own gravity
  (the project's job) on a floor: after 120 steps it rests, y changing < 1e-4 over the last 60,
  on_floor; into a wall head-on: stops, x within 1 mm of touching; at 45°: slides along; a 15°
  ramp: rises; a 55° ramp: does not rise above 0.05; a 0.25 m ledge: stands on it without an
  upward velocity given; off an edge over a gap: falls, on_floor false; all of it again moved by
  (100000, 0, 100000) gives the same positions to 1e-4.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder physics` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^physics/body$'` passes.
