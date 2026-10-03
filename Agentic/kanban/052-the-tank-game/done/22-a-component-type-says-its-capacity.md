# 22 — A component type says its capacity
folder: ecs
after: none
decisions: 0168, 0337

## Change
0337 point 2. Read `ecs/include/ecs/component.h`, `ecs/src/component.c`, `ecs/tests/component.c`,
`ecs/ecs.md` and `ecs/tests/tests.md`.

- `component.h`: add `uint32_t voe_ecs_component_capacity(const voe_ecs_world *world,
  voe_ecs_type type);` beside `voe_ecs_component_count`. Its comment: the capacity the type was
  registered with; with `_count` it tells a caller whether a row will fit before it asks, which
  matters where a full table drops a request silently (ecs/structure.h). An unregistered type
  asserts, as `_count` does.
- `component.c`: carry it out from the table the type already keeps; no new state.
- `tests/component.c`: a check that a type registered with capacity N reports N, before and after
  rows are added, and that `_count` reaching it is the table refusing the next add.
- `ecs.md`: component.h's entry names the capacity among what a type answers.
- `tests/tests.md`: component.c's entry names the capacity check.

## Done when
The test `ecs/component` passes with the new capacity check in it.
