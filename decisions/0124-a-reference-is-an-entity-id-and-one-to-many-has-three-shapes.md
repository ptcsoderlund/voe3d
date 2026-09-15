# 0124. A reference is an entity id, and one-to-many has three shapes

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Amends:** ADR-0123, one paragraph — see *Decision*, point 3.

## Context

**Closes D-236.** ADR-0123 made an entity reference its own declared field type but did
not say what a reference may *name*, and it contradicted itself on lists: its vocabulary
permits *"fixed-size arrays of any of the above"* — so `voe_ecs_entity targets[8]` is
legal — while its rule paragraph says *"One-to-many is inverted, never stored. A flat
component cannot hold a list, and it must not try."* Both sentences are in the same
accepted ADR. The error is the tech lead's, made the same day.

The principal found it from the caller's side, 2026-09-11: *"the player [has] a target
system or a multi-target system. That system should keep track on entities it is
targetting … Two different aspects for referencing."* He had already separated the two
correctly — components of one entity finding each other, versus one entity naming another
— and the third case is what exposed the contradiction.

Fixed by earlier decisions: an entity may not hold two components of one type
(`voe_ecs_component_add` refuses it); removing a component swaps the last row into the
hole, so no row index survives; an entity id is two `uint32_t` carrying a generation, so a
reference to a destroyed entity reads back as absent rather than as whatever moved in.

## Options considered

**What a reference names.**

### Option A — the entity id alone
`voe_ecs_entity`, and the caller states which component type it wants at the point of use.

### Option B — the entity id plus a component type
The reference says *the movement of entity 7* rather than *entity 7*.

### Option C — a row index into the component table
Direct, one fewer indirection.

## Decision

**1. A reference is an entity id, and nothing else. Option A.**

An entity holds at most one component of each type, so the id plus the type named at the
call site is already complete — there is nothing Option B could add that the call site
does not already know, and it would double the size of every reference to carry it.

**2. Option C is not merely rejected, it is unsound here**, and it is worth recording why
so nobody reaches for it as an optimisation: removal swaps the last row into the hole, so
a stored row index silently starts naming a different entity's data. It would work in
every test that does not remove a component.

**3. One-to-many has three shapes, and this replaces ADR-0123's sentence.**

ADR-0123's *"one-to-many is inverted, never stored"* is withdrawn. It contradicted that
ADR's own vocabulary and it had the default backwards. The rule is:

| Shape | Use when | Cost |
|---|---|---|
| **A fixed-size array** of entity references | a natural cap exists — eight targets, four weapon slots | its bytes on every row, and a hard limit that must be handled when reached |
| **Inversion** — the target carries `targeted_by` | each target is claimed by at most one thing | cannot express a target claimed by several |
| **The relationship is an entity** carrying both references, and whatever the link itself knows | unbounded, or the link has its own data — threat, acquisition time, priority | one entity per link |

**The fixed array is the default where a cap is natural; the relationship-as-entity is the
default where one is not. Inversion is the special case.**

**4. Many-to-many is the third shape, and it is a junction table.** The principal reached
this from the relational side — *"entities are substitutes for bridge tables"* — and it is
right one level down: an entity is not itself a junction table, but an entity is what you
make when you need a row of one.

## Blast radius

**Cheap for point 1, moderate for point 3.** Point 1 is forced by the table layout and
there is no plausible reason to revisit it. Point 3 is guidance about which of three legal
shapes to reach for; a component that chose the wrong one is rewritten locally, and only a
fixed array that shipped in saved scenes costs anything to change, because raising a cap
is compatible and lowering one is not.

## Consequences

- **A reference field is eight bytes** and says only *which entity*.
- **A fixed array's cap is a real limit that needs an answer at the call site**, not an
  assertion: the ninth target has to go somewhere or be refused. This is the cost of the
  shape and it should be weighed before choosing it over a junction entity.
- **Inversion being demoted to the special case is a reversal within a day.** It was
  recommended as the default on the strength of "no list can go stale", which is true and
  is not worth the expressiveness it costs when a cap is natural.
- **The editor gets a picker either way.** ADR-0122's description makes a reference field
  a picker and an array of them a list of pickers, with no extra editor code.

## Rejected options and why

**B — reference an entity and a component type.** Rejected: the type is known where the
reference is used, so carrying it doubles the field and adds a way for a stored reference
to disagree with what the caller actually asks for.

**C — a row index.** Rejected as unsound, above. Recorded rather than omitted because it
is the obvious-looking optimisation and its failure is invisible in testing.

## Questions this opens

None. D-236 is closed and ADR-0123's vocabulary stands unchanged.
