# 03 — The world owns which rows exist
folder: ecs
decisions: 0168, 0190, 0193

## Change
0190 in `ecs`: a default row per type, a structural queue per world, and what a type needs (0193).

`include/ecs/component.h` (+ `src/component.c`):
- `void voe_ecs_component_default_set(voe_ecs_world *world, voe_ecs_type type, const void *row)`: copies
  `row` (the type's size) into memory the world pushed for it. Once per type; a second call asserts.
- `const void *voe_ecs_component_default(const voe_ecs_world *world, voe_ecs_type type)`: NULL when none was set.
- `void voe_ecs_component_needs_set(voe_ecs_world *world, voe_ecs_type type, voe_ecs_type needed)`: rows of
  `type` do nothing without a row of `needed` on the same entity. It is stored and never read by `ecs`. Once per
  type.
- `bool voe_ecs_component_needs(const voe_ecs_world *world, voe_ecs_type type, voe_ecs_type *out)`: false when
  none was set.
- Rewrite the header's creation paragraph to 0190's rule 3: values by the owner's intent, rows added and removed
  through the structural queue, the creation exceptions standing.

New `include/ecs/structure.h` (+ `src/structure.c`, and `src/world_internal.h` as needed): the world's
structural queue.
- `voe_ecs_limits` (world.h) gains `uint32_t structure_requests` and `uint32_t structure_bytes`. Unlike the
  other limits, zero is allowed. A world made with zero has no queue, and every submit to it returns false.
  Existing callers pass nothing and are unchanged.
- `[[nodiscard]] bool voe_ecs_structure_add(voe_ecs_world *world, voe_ecs_type type, voe_ecs_entity entity,
  const void *row)`: copies the row's bytes into the queue.
- `[[nodiscard]] bool voe_ecs_structure_remove(voe_ecs_world *world, voe_ecs_type type, voe_ecs_entity entity)`.
- `[[nodiscard]] bool voe_ecs_structure_destroy(voe_ecs_world *world, voe_ecs_entity entity)`.
  Each returns false only when the queue has no room (requests or bytes).
- `void voe_ecs_structure_apply(voe_ecs_world *world)`: applies every request in submission order, then empties
  the queue. An add whose entity is dead, already has that type, or whose table is full is dropped. A remove with
  no row and a destroy of a stale id are dropped. Nothing is reported.
- `uint32_t voe_ecs_structure_count(const voe_ecs_world *world)`: requests waiting.
- The header says who may submit (anyone), that the program calls `_apply` once a frame before its systems run
  so no system sees a row appear or vanish mid-run, and why the drops are silent (the queue's ordinary cost, as
  an intent naming a dead entity is).

Update `ecs.md`, `src/src.md` and `tests/tests.md`.

## Done when
The folder's check passes (`checks.sh` for `ecs`) with:
- new `tests/structure.c`: an add lands only at `_apply`, with the given bytes; a remove and a destroy likewise;
  requests apply in submission order (add then remove leaves nothing; destroy then add leaves nothing); each drop
  case above is dropped without asserting; a full queue (requests and bytes separately) returns false; a
  zero-limit world refuses every submit; `_count` is 0 after `_apply`.
- `tests/component.c`: a default set is read back byte for byte, and an unset type gives NULL. A needs set is
  read back, and an unset type gives false.
