# 053 — A component type names its replace intent

claimed-by: -
blocked-by: -
status: todo
decision: *An edit is a replace intent, and the owning system applies it* (ADR-0134) points 1–3 — the intent's value holds the entity at offset zero and a whole row at an offset the declaring folder states; `ecs` stores the pair beside the description and never reads either; it is its own call, not a parameter to registration, and a type without one is shown and not editable.

## Goal

A caller holding a world and a component type can find the intent that replaces a whole row
of it, where the row sits in that intent's value, and how big both are.

## Scope

**1. `ecs/include/ecs/component.h`.**

```c
void voe_ecs_component_replace_set(voe_ecs_world *world, voe_ecs_type type,
				   voe_ecs_intent intent, size_t row_offset);

typedef struct {
	bool set;
	voe_ecs_intent intent;
	size_t row_offset;
	size_t row_size;    // the component's row, as registered
	size_t value_size;  // the intent's value, as registered
} voe_ecs_replace;

voe_ecs_replace voe_ecs_component_replace(const voe_ecs_world *world, voe_ecs_type type);
```

- `_replace_set` asserts on three things and no more: the type already has one;
  `row_offset` is below `sizeof(voe_ecs_entity)`; `row_offset` plus the row's size exceeds
  the intent's value size. Those are comparisons of sizes `ecs` already holds.
- `_replace` on a type that never had one: `set` false and everything else zero.
- `component.h` includes `ecs/intent.h` for `voe_ecs_intent`; `intent.h` includes only
  `ecs/world.h`, so there is no cycle.
- The header says, in its own style: the entity is at offset zero; the declaring folder gives
  `row_offset` with `offsetof`; `ecs` stores the pair and never reads a value; a type without
  one is shown and not editable.

**2. `ecs/src/world_internal.h`, `ecs/src/component.c`.** `struct voe_ecs_table` gains the
intent, the offset and whether they were set. Nothing else changes shape.

**3. Tests — `ecs/tests/component.c`.**

- A type given a replace intent hands back the same intent, the offset, the row's size and
  the intent's value size.
- A type never given one reports `set` false.
- **The round trip the editor will make, once by hand**: a local intent type
  `{ voe_ecs_entity entity; <the test's row type> row; }` registered and named; a row read
  with `voe_ecs_component_get`, copied into a zeroed value at `row_offset`, one byte changed,
  submitted; the test drains it itself with `voe_ecs_component_set`; the row changed in that
  byte and no other.

**4. `ecs/ecs.md`** — the `component.h` entry mentions the replace intent.

## What must not change

- `voe_ecs_component_register`'s signature and every existing call site. This is additive;
  no folder but `ecs` is edited.
- `ecs` reads no intent value and no description, and includes no `math`, `scene` or `3d`
  header.
- No query, no bulk helper.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R ecs` passes.
- `git diff --stat` touches only `ecs/`.
- `grep -rn 'math/\|scene/\|3d/' ecs/include ecs/src` returns nothing.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A test that edits one byte of a component through its replace intent, naming the
component's type only where it sets it up.

## Notes
