# 050 — The world lists its component types, and a registration carries a description

claimed-by: -
blocked-by: 048
status: todo
decision: *The world lists its component types, and a description travels with a registration* (ADR-0132) — the world can say how many types are registered, the type at an index, and the key behind a type; `voe_ecs_component_register` takes the struct description the declaring folder wrote and hands it back for a type; NULL means undescribed and is allowed. `ecs` stores the pointer and never reads it. Why a struct and its description are written once together is ADR-0122; where the description type lives is ADR-0127.

## Goal

A caller holding only a world and an entity can find out what that entity is made of, and
for each type it has, reach the field list of its struct. Nothing in the engine names a
component type to do it.

## Scope

**1. `ecs/include/ecs/component.h` — three accessors and one changed signature.**

- `voe_ecs_component_type_count(world)` — how many types have been registered.
- `voe_ecs_component_type_at(world, index)` — the type at that index. Registration order
  is the enumeration order; asserts on an index at or past the count.
- `voe_ecs_component_key(world, type)` — the `voe_ecs_key` the type was registered
  against, so a caller has a name to show.
- `voe_ecs_component_register` gains a final parameter: a pointer to the struct record
  `base/describe.h` exposes (card 048 names the type). **NULL is allowed and means the
  component is undescribed** — it is not an assert and not a failure.
- `voe_ecs_component_description(world, type)` — the pointer that was registered, or NULL.
- Document in the header, in the style already there, that `ecs` stores the description
  and never reads it: the world does not know what a field is.

**2. `ecs/src/component.c`, `ecs/src/world_internal.h` — store it.**

One extra pointer per registered type beside the key. No other structure changes.

**3. Every registration call site passes its description or NULL.**

- `scene/src/transform_system.c` — passes the transform's description, which card 048
  creates. **This is the only one that passes a real pointer in this card.**
- `scene/src/camera_system.c`, `scene/src/light_system.c`, `3d/src/mesh_component.c`,
  `3d/src/material_component.c`, `3d/src/panel_component.c` — pass NULL. Describing them
  is a later card.
- `ecs/tests/component.c` — its three registrations pass NULL except where a test below
  needs otherwise.

**4. Tests — `ecs/tests/component.c`.**

- Registering three types and enumerating them returns three, in registration order, with
  the right key behind each.
- The reverse lookup: an entity given two of three types, walked over the full type list
  with `voe_ecs_component_get`, is found to have exactly those two — this is the loop an
  inspector performs, written once here to prove it works.
- A type registered with a description hands the same pointer back; a type registered with
  NULL hands back NULL.
- A stale entity id finds nothing in that walk.

## What must not change

- **No bulk helper.** There is no `voe_ecs_entity_types(...)` filling an array. The walk is
  the caller's loop; a convenience waits for a second caller.
- **No query API.** Nothing that answers *every entity with these components*. That is a
  separate open question and this card does not touch it.
- **No bitmask, no archetypes, no per-entity type set.** The direct index per table stands
  exactly as `component.h` describes it.
- **`ecs` gains no dependency beyond `base`**, which it already has. It must not include a
  `math` or a `scene` header, and it must not read a field record's contents.
- **No editor, no inspector, no serializer, no identity component.** Later cards.
- Row order, the removal swap, generation checks and every existing function's behaviour
  are unchanged.

## Verify

- Linux: `cmake -P check.cmake` green.
- `ctest` green, including the new cases.
- Build with descriptions off and with them on: green both ways, and with them off the
  transform registers NULL and the reverse-lookup test still passes.
- `grep -rn 'math/\|scene/\|3d/' ecs/include ecs/src` returns nothing.
- From the planning root, `sh tools/hot.sh` reports no new `OVER`.

## Done looks like

A test that takes an entity, asks the world what it is made of, and gets the answer
without naming a single component type — and for the transform, reaches its field list from
that answer. That test is the inspector, minus the drawing.

## Notes
