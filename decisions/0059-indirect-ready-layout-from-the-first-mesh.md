# 0059. The 3D renderer walks the tables and draws from an indirect-ready layout, from the first mesh

- **Status:** Accepted
- **Date:** 2026-09-03
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-027

## Context

Card 018 is the first card that produces many things, and so the first `3d`
card. ADR-0030 deferred `3d`'s internals to it: does `3d` keep its own per-frame
layout, run an extract step from `scene` into `render` commands (Bevy's shape),
or walk the ECS tables directly?

A second question surfaced while the card was written, raised by the principal:
**one draw per mesh — is that not expensive?** Grass, particles and "many
objects" are a feature of an ECS engine, not an optimisation, and the principal
asked whether meshes sharing a shader and texture ids should share a draw call.

Constraints already fixed:

- **ADR-0016 — GPU-first.** The renderer is *architected* for GPU-side work from
  the start, not retrofitted. Its stated consequence: entity data in one large
  GPU buffer indexed by id, and indirect draws issued from GPU-written commands.
- **ADR-0018 — GPU resources by generational id**, the id being the index the
  shader uses. Descriptor indexing and buffer device address are core in the
  Vulkan 1.3 floor.
- **ADR-0007 / 0011 / 0013 / 0017** — flat component tables, one writer, reads
  direct and `const`, writes as queued intent.
- **ADR-0034 — implement on demand**, and **ADR-0055** — nothing is built that
  nothing measures.
- **ADR-0033** — reverse-Z depth. Opaque visibility is resolved per pixel by
  the depth buffer, so the *order and grouping* of opaque draws does not change
  the image. Only blended geometry is order-dependent (D-057, open).

## Options considered

### Option A — per-mesh buffers, one draw per mesh
Each mesh owns a vertex buffer and an index buffer; the model matrix is a push
constant (card 014's shape); one `vkCmdDrawIndexed` per mesh. Simplest. Moving
later to a shared pool and indirect draws rewrites the mesh component, the upload
path and the draw — the retrofit ADR-0016 said not to plan on.

### Option B — indirect-ready layout, plain draws
All geometry lives in **one shared vertex pool and one shared index pool**; a
mesh is a *range* in each. Per-object data — world matrix and material index —
lives in **one GPU buffer indexed by object number**, written once per frame.
Textures are bindless by id. The CPU still issues one draw per mesh, so the
indirect command buffer and any culling are not built. Switching to a single
`vkCmdDrawIndexedIndirectCount` later changes the draw loop and nothing else.
Costs: a pool allocator (a bump allocator suffices until something frees), and a
shader that reads per-object data from a buffer rather than a push constant.

### Option C — fully GPU-driven now
Indirect command buffer, compute culling, GPU-written draw list. Nothing to cull
in a scene of one model; no measurement asks for it.

### On the extract step (Bevy's shape)
Copying `scene` components each frame into a `render`-owned layout buys
decoupling of simulation and render threads. There is one thread. The copy is
work with no consumer. Rejected as premature under ADR-0034; the per-object GPU
buffer in Option B *is* the extract, written straight from the tables.

## Decision

**Option B.** The deciding factor is ADR-0016's own sentence: architected from
the start, not retrofitted. The *layout* is the part that is expensive to
change; the single indirect call is the part that is cheap to add. So the layout
is fixed now and the call is deferred.

Concretely, for card 018 and after:

1. **`3d`'s draw system walks the component tables directly.** No extract step,
   no second CPU-side layout. It reads `transform`, `mesh` and `material`
   `const`, per ADR-0013.
2. **Geometry lives in a shared pool**, one vertex pool and one index pool owned
   by `render`. A `mesh` component is a range: first vertex, first index, index
   count. Uploading a mesh appends to the pool.
3. **Per-object data lives in one GPU buffer**, one record per drawn object per
   frame slot (ADR-0050): world matrix and material index. The shader reads its
   record by object number. **Card 014's push constant goes away**; push
   constants remain available for what is genuinely per-frame and tiny.
4. **Materials live in one GPU buffer too**, one record per material: factors
   and texture ids. The shader reads the material by index and the texture by id.
5. **One draw per mesh is issued from the CPU**, walking the tables in table
   order. No sorting, no instancing, no indirect buffer, no culling. All four
   become a later card, each one a change to the draw loop only.
6. **Grouping is the engine's, never the user's.** Opaque geometry needs no
   grouping to be correct — the depth buffer resolves it — and the grouping key
   for a batch is "same pipeline", which the engine knows better than a person.
   No editor feature for hand-grouping objects into batches or buffers is
   planned. What the editor may do later is *show* batches as a diagnostic.
7. **Blended geometry is outside this ADR.** It is order-dependent, cannot be
   batched arbitrarily, and is D-057's question.

## Blast radius

**Moderate, and this is the point.** The layout — pools, ranges, a per-object
buffer, a material buffer — is what every later renderer feature stands on:
instancing (grass), GPU-written objects (particles), indirect draws, culling.
Reversing it means going back to per-mesh buffers, which nothing will want.
Everything deferred above is additive.

## Consequences

- **The mesh component holds ranges, not buffer ids.** ADR-0018's ids still
  name the pools themselves and every texture; a mesh is located *within* an id.
- **The pool needs an allocator.** A bump allocator that never frees is
  sufficient for card 018 and stated as such; freeing and compaction arrive when
  a card unloads something.
- **A pool has a capacity, given at creation** (ADR-0032's arena shape, on the
  GPU). Running out is a returned failure, not a reallocation, until a card
  needs growth.
- **The shader changes shape now rather than later**: it reads a per-object
  record and a material record from buffers and indexes textures by id. Card
  018 pays this once.
- **Grass and particles fit without an undo.** Grass is one range drawn many
  times with many per-object records; particles are per-object records the GPU
  itself writes (ADR-0016's GPU-resident truth).
- **What this does not buy, said out loud:** draw-call count does not fall in
  card 018, and nothing runs on a second thread. The data is merely ready.

## Rejected options and why

- **Option A** — the retrofit path ADR-0016 rejected in advance; cheapest today
  and the most expensive to leave.
- **Option C** — nothing to measure against; ADR-0055 forbids building it on a
  hope.
- **An extract step** — a copy with no consumer while there is one thread; the
  per-object GPU buffer already is the extracted form.
- **User-grouped batches in the editor** — work the engine does automatically
  and better, and a wrong grouping would be invisible.

## Questions this opens

- **When the single indirect call is switched on**, and what number triggers it.
  Register row, trigger: the first card that draws hundreds of objects (grass or
  particles).
- **Pool freeing and compaction.** Trigger: the first card that unloads a model.
- **D-057 (transparency)** now has one more consumer — anything blended must
  leave the pooled opaque path and be sorted.
