# 056 — The transform names its replace intent and settles its rotation

claimed-by: -
blocked-by: 053, 054, 055
status: todo
decision: *An edit is a replace intent* (ADR-0134) points 1 and 5; *A drain corrects a bad value or keeps the last valid one, and says so* (ADR-0138) — correct where one nearest valid value exists, keep the last valid row where none does, report on the edge; typed calls keep only the checks they already have.

## Goal

The transform's existing intent is its replace intent, and its drain never writes a
rotation that is not a rotation.

## Scope

**1. Registration, `scene/src/transform_system.c`.** After the intent is registered,
`voe_ecs_component_replace_set(world, type, intent, offsetof(voe_scene_transform_intent,
transform))`.

**2. The drain settles each intent before `voe_ecs_component_set`.** The rules, written in
`scene/include/scene/transform_component.h` beside the struct:

- **A component of position, rotation or scale that is not finite** → the last valid row is
  kept. `error:`.
- **A rotation whose length is below `VOE_SCENE_TRANSFORM_LEAST_ROTATION` (1e-6)** → there is
  no nearest rotation; the last valid row is kept. `error:`.
- **A rotation whose length is off 1 by more than `VOE_SCENE_TRANSFORM_ROTATION_TOLERANCE`
  (1e-3)** → normalised with `voe_math_quat_normalize`. `warning:`.
- **Within the tolerance** → normalised and not reported: that is rounding.
- Scale has no rule; `3d/src/normal_matrix.c` already answers a zero scale.

**3. Reporting.** On the first drain that settles an intent — a silent normalisation does not
count — after a drain that settled none: one line on stderr naming the system, the entity,
the field, the value submitted and what was applied, e.g.
`warning: voe_scene_transform: entity Cube_2: rotation had length 0.62, normalised` or
`error: voe_scene_transform: entity 7v1: rotation had no length, kept the last valid transform`.

- The entity is named by its identity's name where it has one, else `<index>v<generation>`.
  **A world may have no identity table** — `dev` registers none — so find the identity's type
  by walking `voe_ecs_component_type_count` and `_type_at` and comparing
  `voe_ecs_component_key` with `&voe_scene_identity_key`, never through a lookup that asserts.
- When a drain settles none after such a run: `<word>: voe_scene_transform: <n> intents
  settled in that run`, the word `error` if any intent in the run was kept, else `warning`.
- The run flag and count are file-scope statics, documented as in card 055.

**4. Tests — `scene/tests/transform.c`.**

- `voe_ecs_component_replace` for the transform is `set`, with the offset equal to
  `offsetof`.
- A rotation of length 2 arrives unit length and the same rotation; one of length 1.0005
  arrives unit length; a zero rotation and a NaN position both leave the previous row
  untouched.
- An intent submitted raw through `voe_ecs_intent_submit` is settled the same way as one
  through `voe_scene_transform_submit`.

## What must not change

- **No new assert in `voe_scene_transform_add` or `voe_scene_transform_submit`.** `3d`'s
  importer adds transforms through them.
- The intent's type and layout, last writer wins, and an intent for a destroyed entity being
  dropped.
- Camera, light and identity; `math` and `ecs`.

## Verify

- Linux: `cmake -P check.cmake` green.
- `ctest -R scene` passes with descriptions off and on, as card 055.
- Run `voe_dev`: the turning cube turns as before and no `warning:` or `error:` line appears.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A transform whose rotation cannot be written wrong through any door, and a person who
dragged it wrong reads one line saying so.

## Notes
