# 0132. The world lists its component types, and a description travels with a registration

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-234**, opened by ADR-0123 the same week and marked on the critical path: *how
the world enumerates its registered component types, and how a caller asks what an entity
has.*

An inspector shows a selected entity and lets a person edit it. It cannot be written
today. `voe_ecs_component_get` answers *does this entity have type T* for a `voe_ecs_type`
the caller already holds, and `voe_ecs_component_entities` answers *who has T* for one
table — but a `voe_ecs_key` is an address chosen by the registering module (ADR-0007's
deliberate absence of a central number list), and the world exposes no list of what was
registered. **The open question — what is this entity made of — has no caller-side
answer at all.**

The other half is what the answer is worth once you have it. ADR-0122 requires a struct
and its description to be written once together; ADR-0127 puts the description mechanism
in `base/include/base/describe.h` and is explicit that it describes **structs, not
components**, with `ecs` named as *one caller among possible others*. Card 048 builds it
and describes the transform. So the field lists will exist in the declaring folders —
what is undecided is how a caller holding a `voe_ecs_type` reaches one.

Constraints already fixed. ADR-0007: flat tables, handles, the plain version chosen
deliberately over archetypes. ADR-0121: the editor is a leaf and **no folder may depend on
it**. ADR-0125: identity is an optional component, so the set of authored entities is
already one table's entity array and needs nothing new. ADR-0126: editor-only components
exist and the cook strips them. `ecs` already depends on `base`.

The scene tree is therefore free and only the inspector is blocked.

## Options considered

### Option A — enumeration only, and the editor keeps its own map
Three accessors: how many types are registered, the type at an index, and the key behind a
type. The editor walks all of them calling `voe_ecs_component_get` — a handful of O(1)
lookups per selected entity, free at this scale. Nothing else changes. The editor then
holds its own table from key to field list, because nothing connects the two.

### Option B — enumeration, and a description carried on the registration
The same three accessors, plus registration takes the declaring folder's struct
description, and the world hands it back for a type. The inspector becomes: for each
registered type, does this entity have it; if so, here is the row and here is what its
fields are.

### Option C — a bitmask per entity
Each entity slot keeps a 64-bit word of which types it has. *What does this entity have*
is one load; *every entity with these components* is a mask test, which is most of D-171.

## Decision

**Option B.** The deciding factor: it is the only one where adding a component to any
folder makes it appear in the inspector, correctly, with **no edit anywhere else** — which
is the whole point of ADR-0122's write-once rule, carried the last step to its consumer.

1. **The world lists its component types.** How many are registered, the type at an index,
   and the `voe_ecs_key` behind a type. Registration order is the enumeration order and
   nothing promises anything further about it.
2. **`voe_ecs_component_register` takes a struct description**, the one the declaring
   folder wrote through `base/describe.h`, and the world hands it back for a type.
   **NULL is allowed and means undescribed** — a component nothing has described yet is a
   component the inspector shows by name and does not expand, not a build failure.
3. **`ecs` includes `base/describe.h` and gains no dependency**, because `ecs` already
   depends on `base` and ADR-0127 built the description type to be reachable from exactly
   here.
4. **`ecs` still knows nothing about any component's meaning.** It stores a pointer it was
   handed and does not read it. Nothing in `ecs` includes `math`, and the enum of kinds is
   `base`'s.
5. **No bulk helper.** Asking *what does this entity have* is the caller's loop over the
   type list. A fill-an-array convenience waits for a second caller (rule 10).
6. **Enumeration lists editor-only types too**, which is right: the editor is where they
   are inspected, and the cook is what removes them (ADR-0126).

## Blast radius

Small. The accessors are additive. The registration signature changes at six engine call
sites and three tests, in one card, which is ordinary. What would be expensive to reverse
is **point 2's direction**: once folders hand their descriptions to the world, the world
is the place a tool asks, and moving that map back out into each consumer would mean
rebuilding it in every consumer. That is the cost being bought deliberately.

Reversibility: **cheap for the accessors, moderate for the description on registration.**

## Consequences

- **The inspector can be written**, and it names no component type. A folder that adds a
  component gets editor support by describing it, which it was already required to do.
- **A component with no description is visible and not expandable.** That is a deliberate
  soft edge: it keeps the change from being a flag day, and it makes the gap obvious in
  the tool rather than at the compiler.
- **Every registration call site grows an argument** — `scene`'s transform, camera and
  light, `3d`'s mesh, material and panel, and the `ecs` tests. Per ADR-0113 the card owns
  them.
- **`ecs` carries one pointer per registered type** and nothing else grows.
- **D-171 is not answered and was not meant to be.** The inspector is a reverse lookup on
  one entity, not a join; the query API keeps its own condition.
- **The description switch reaches further than `scene`.** Card 048's compile-time toggle
  now also decides whether registrations pass a real pointer or NULL, which is the
  mechanism by which a shipped build carries no descriptions at all.

## Rejected options and why

**A — enumeration only.** Cheaper today, and its bill arrives forever afterwards: the
editor holds a hand-maintained list of every component type in the engine, and the failure
mode is silent — you add a component, and it simply does not show up, months later, for
someone who does not know the list exists. That is precisely the drift ADR-0122 was
written to make impossible, reintroduced one layer up.

**C — a bitmask.** Solves a problem nobody has. Thirty O(1) lookups once per selected
entity per frame is not a cost worth a new data structure, and the mask caps component
types at its width permanently. It is also a query engine arriving before anything
queries, which ADR-0007 declined once and the register has declined twice since.

## Questions this opens

- None. D-171 and D-097 keep their existing conditions.
