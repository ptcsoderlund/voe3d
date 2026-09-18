# 0085. A mesh's geometry is replaceable by a direct call, and it is the only mutable field on the table

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-121

## Context

ADR-0084 gave the engine a transient geometry pool, so a mesh can now be rebuilt
every frame without stalling the card or exhausting a pool. **That is necessary
and not sufficient**, and the gap was found while writing the card rather than
while taking the decision.

Everything this engine draws is an entity. `voe_3d_draw_system_run` walks the mesh
table and issues one draw per row. The table is
`3d/include/3d/mesh_component.h`, a row is `{ voe_render_geometry geometry;
voe_3d_layer layer; }`, and **its only writer is `voe_3d_mesh_add`**, whose
comment reads:

> It is a direct call and not an intent because it is creation — see the header on
> why there is no system here at all.

A per-frame text block is not creation. So there is no way for a transient
geometry id to reach the screen: it is made, it is valid for the frame, and
nothing can put it in the row the draw system reads.

Constraints already fixed:

- **ADR-0011 — component and system are paired**, and a component's writes go
  through its system.
- **ADR-0017 — no system-to-system dependencies**; writes are queued as intent and
  applied by the one writer, reads are direct and `const`.
- **ADR-0065 — one loop, named phases, one thread.** `update` runs before `draw`,
  and an intent queue in this engine is drained explicitly by its system rather
  than applied automatically at a frame boundary (`ecs/include/ecs/intent.h`), so
  a value submitted during `update` can be visible to `draw` in the same frame.
- **ADR-0084 — a transient id expires at the end of the frame.** Whatever writes
  it into the table must therefore run in the same frame, every frame.
- **The mesh table has no system at all today**, which is why the direct-write
  exception for creation exists in the first place.

## Options considered

### Option A — a direct setter: `voe_3d_mesh_set_geometry`

One function beside `voe_3d_mesh_add`, taking the world, the entity and the new
geometry id, returning false when the entity has no mesh. The caller writes it in
the `update` phase; the draw system reads it in `draw`.

- **Costs:** it widens the direct-write exception on this table from *creation* to
  *creation and one field*, so the header's sentence has to be rewritten rather
  than extended. It leaves ADR-0011's pairing rule with a second table-shaped
  exception on the books.
- **Makes easy:** the whole changing-text path, in one function nobody has to
  learn a mechanism for. A HUD is: build the block, set the geometry, done.
- **Makes permanent:** the precedent that a component with no system may be
  written directly. That is already the precedent — this makes it explicit and
  bounded to one named field instead of implicit and bounded to one named call.

### Option B — an intent and a system in `3d`

A `voe_3d_mesh_geometry_intent`, submitted by anyone, drained by a new mesh system
that runs in `update` before the draw system.

- **Costs:** a system whose entire job is to copy one field, a queue sized for the
  worst case, and a per-frame ordering requirement between two systems in `3d` —
  which is the shape ADR-0017 exists to keep out. It is also the only system in the
  engine that would have to run every frame for the picture to be *correct* rather
  than merely current.
- **Makes easy:** conformance with ADR-0011 as written, and a natural place to put
  later mesh mutations if there are any.
- **Makes permanent:** a system in `3d` that everything drawing changing geometry
  has to know to run.

### Option C — the layer field too, or the whole row

Make the row wholly writable rather than one field.

- Nothing asks for it. The layer is described as a property that does not change —
  *"moving something does not move it between them"* — and a writable layer is a
  way to get the depth-clear ordering wrong that nothing currently offers.

## Decision

**Proposed: Option A**, and the deciding factor is that **Option B is a system
that exists only to copy a field, and the mesh table has deliberately had no
system since it was created.**

The engine already accepts that this table is written directly; ADR-0011's pairing
rule is about a component whose writes need coordinating between systems, and
nothing coordinates here — the mesh row has exactly one writer per entity, which
is whatever created it.

What this pins:

1. **`voe_3d_mesh_set_geometry(world, entity, geometry)` exists**, returns false
   when the entity has no mesh or is not alive, and is a direct call.
2. **Geometry is the only mutable field.** The layer is not writable and the row is
   not replaceable wholesale.
3. **The header's reasoning is rewritten, not extended.** The sentence explaining
   why the table has no system must now cover both calls and say what the boundary
   is: this table has one writer per entity and nothing to coordinate, so it is
   written directly; the day two systems both want to write a mesh row, that is an
   ADR and not a patch.
4. **A transient id in a mesh row is the caller's promise to rewrite it every
   frame.** Nothing enforces it. A row left holding last frame's transient id
   names a slot whose generation has moved, so the draw refuses it and logs — the
   picture loses that object rather than drawing garbage. That is the intended
   failure and it is worth stating out loud.

## Blast radius

**Cheap.** One function on one table, and the alternative stays available: if a
mesh system is ever needed for another reason, the setter becomes its intent with
no call-site churn worth naming.

The thing that could go wrong is scope creep on the table — a second mutable
field, then a third, until it has a system's worth of writes and no system.
Point 2 is the guard, and the third field is the trigger to revisit.

Reversibility: **cheap.**

## Consequences

- **Card 028 becomes claimable**, and the changing statistics readout has a path
  to the screen.
- **ADR-0011's pairing rule now has a written exception** rather than an implied
  one. That is an improvement in honesty and a small loss in uniformity.
- **A silent failure mode exists that did not before:** a row holding a stale
  transient id. It is loud in the log and invisible in the picture except as a
  missing object, which is the wrong way round for finding it quickly.
- **Nothing stops a caller putting a transient id on a long-lived entity and
  forgetting it.** The guard is the log line, and D-120 already asks the adjacent
  question about the transient pool being misused.

## Rejected options and why

**Option B — intent and a system.** Rejected because it builds a system to copy a
field, and because a per-frame ordering requirement between two `3d` systems is
the exact coupling ADR-0017 was written to prevent. It would be the right answer
the moment a second writer exists; there is none.

**Option C — the whole row mutable.** Rejected as unasked-for, and because a
writable layer is a new way to break the depth-clear ordering that ADR-0074 spent
a card getting right.

## Questions this opens

- **D-122 — what a stale transient id in a component should do.** Today's answer
  falls out of the generation check: refuse and log. Whether a component holding
  an expired id deserves louder treatment — an assert in a debug build, a count in
  the frame statistics — is a real question and this ADR does not answer it.
