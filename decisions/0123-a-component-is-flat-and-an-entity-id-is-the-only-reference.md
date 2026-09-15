# 0123. A component is flat, and an entity id is the only reference

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** in part, by ADR-0154 — point 5: a fixed-size array may hold arrays, rectangular and of one kind

## Context

**Closes D-232**, opened by ADR-0122 the same day: the vocabulary of field types a
component description can express. ADR-0122 settled that a component describes itself and
that the struct and the description are written once together; it could not say what a
field may *be*.

The principal, 2026-09-11: *"I am sure that I only want one dimensional components … only
primitives and no pointers. I just don't know how to solve the references."*

**The rule is already in force and this ADR states it rather than introducing it.**
`ecs/include/ecs/world.h` says *"Components being plain data with no pointers in them is
what makes a later card able to read them from another thread"*, and
`ecs/include/ecs/component.h` says *"Removing swaps the last row into the hole"* — so a
pointer into a component table dangles at the next removal. Flatness is load-bearing for
ADR-0119's loader thread and forced by the table layout.

Also fixed: ADR-0122's description reads a field's offset from `offsetof`, so a field must
have a layout the compiler knows. Entity ids are `{uint32_t index; uint32_t generation;}`
and the header invites storing them — *"Copy it, store it, compare it."* An entity may not
hold two components of one type; `voe_ecs_component_add` refuses it.

## Options considered

### Option A — primitives only, no arrays
The strictest reading. Every field is a scalar. A name would have to live outside the
component entirely.

### Option B — primitives, fixed-size arrays, and entity references
Primitives and fixed-size arrays of them; an entity reference as its own declared type.
Nothing that allocates, nothing that points, nothing that nests.

### Option C — B plus nested component structs
Allow a field whose type is another described struct. More expressive; the description
becomes a tree and the inspector becomes recursive.

## Decision

**Option B**, the principal's call, including fixed-size arrays at his explicit request.

**The vocabulary.** A described field is one of:

1. **Numbers** — the sized integer and floating-point types.
2. **Booleans.**
3. **Vectors and quaternions** — the fixed shapes `math` already owns.
4. **Enumerations**, authored by name and stored as an integer.
5. **Fixed-size arrays** of any of the above, including `char` arrays, which is how a
   component carries a name.
6. **An entity reference.**

**Nothing else.** No pointer, no nesting, no container that allocates, no string that owns
its own memory.

**An entity reference is its own declared type, not two integers.** Declared as two
integers it still *works* at runtime and fails silently in the two places that matter: the
serializer will write a runtime slot number that means nothing after a reload instead of
remapping it, and the inspector will offer two number boxes instead of a picker. Both
failures are quiet, which is why the type exists.

**One-to-many is inverted, never stored.** A flat component cannot hold a list, and it
must not try. A child holds its parent; a parent holds nothing. Children are found by
walking the table. This is also the more correct shape: destroying a child leaves no stale
entry anywhere, because there was never a list.

**Many-to-many is an entity.** Where a relational schema would add a junction table, here
the relationship becomes an entity carrying a component with both references. The
principal reached this from the database side — *"entities are substitutes for bridge
tables"* — and it is right one level down: an entity is not itself a bridge table, but an
entity is what you make when you need a row of one.

## Blast radius

**Load-bearing.** Every component ever written, every scene file ever saved and every
inspector row is shaped by this list. Adding a type to the vocabulary later is cheap and
backwards-compatible; **removing one is not**, because saved scenes already contain it.
Widening to Option C later is possible and would not invalidate existing files.

## Consequences

- **A component is memcpy-able, comparable, and writable as text field by field**, with no
  allocation on either side. That is what the serializer, the loader thread and the
  editor's undo all want, and they get it for free.
- **There is no garbage to collect, because nothing owns anything.** The world lives in an
  arena with no destroy; freeing is rewinding. This ADR is what keeps that true.
- **Fixed-size arrays cost their bytes whether used or not**, on every row of the table.
  A 32-byte name on a 4096-entity table is 128 KB spent on names. Accepted knowingly; a
  later interning scheme is a change to one field type, not to the model.
- **A name is now a component's business.** Where the name lives — every component, or one
  identity component — is not decided here.
- **Entity references need remapping on load**, and this ADR does not say how. D-098 owns
  it and is now on the critical path.
- **The inspector cannot yet be written.** Answering *what components does this entity
  have* needs the world to enumerate its registered types, and it has no such call — a
  `voe_ecs_key` is a name pointer and the world exposes no list (D-234).

## Rejected options and why

**A — no arrays.** Rejected by the principal directly. Without them a name has to live
outside the component, which means a parallel structure keyed by entity that is not a
component, is not described, is not serialised by the same path and is not visible to the
editor — reintroducing exactly the special case this model exists to remove.

**C — nested structs.** Rejected as premature. It makes the description a tree, the
inspector recursive and the text format nested, and nothing has asked for it: `math`'s
vectors cover the one case that would have. It can be added later without invalidating a
saved file, which is the property that makes deferring it safe.

## Questions this opens

- **D-234** — how the world enumerates its registered component types, and how a caller
  asks what an entity has. Nothing can inspect an entity without it.
- **D-235** — where an entity's authored name and stable id live: a field on every
  component, or one identity component every authored entity carries.

**D-098** (entity identity and cross-file references) moves onto the critical path: it is
now the last thing between this decision and a scene that can be saved and reloaded.
