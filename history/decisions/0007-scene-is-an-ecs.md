# 0007. The scene is an entity component system

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The capability list assumed a scene — "load a model, move a camera through the
scene" — without ever naming what a scene *is*. That omission was surfaced by
taking The Machinery (Our Machinery, C, modular, closed 2022) as a reference
point.

Two constraints were already fixed and neither had been applied to this
question:

- `guidelines.md`: **"Data driven design is #1 choice"**, and composition
  preferred over inheritance and interfaces.
- `coding_convention.md`: **`**` is forbidden.** One level of dereference only.

## Options considered

### Option A — Node tree
The world is a hierarchy of objects that own their parts. Conventional, familiar,
what glTF itself describes.

### Option B — Entity component system
Entities are identifiers; components are plain data in tables; behaviour is
functions sweeping those tables. What The Machinery and Bitsquid did.

## Decision

**Option B.** The plain version — flat component tables and handles — not an
ambitious archetype engine.

The deciding factor is that the principal's own guidelines already argued it.
"Data driven first, composition first" *is* the ECS argument; a node tree would
have contradicted a written rule to satisfy familiarity.

A second factor confirmed it: ADR-0009 makes text-based authoring a requirement
for AI-agent-editable content. Flat component tables serialise to text almost
directly. A pointer-linked node tree does not.

## Blast radius

**Reversibility: load-bearing.** The scene model is the spine every other folder
touches. This is not reversed; it is superseded by a rewrite.

## Consequences

### Component ownership — reconciling with `guidelines.md`

`guidelines.md` states that folders own their data and **mutation from outside is
forbidden**. Taken naively this forbids ECS outright, because systems write to
component data continuously. The reconciliation, which is binding:

> **A component type is owned by the folder that defines it, and only that
> folder's code mutates it. The ECS folder owns the containers, not the
> contents.**

Without this rule written down, the first system to touch a `Transform` violates
`guidelines.md`. With it, a transform system mutating transforms is the owner
mutating its own data, and the guideline holds unchanged.

### No double indirection — reconciling with `coding_convention.md`

The usual archetype ECS stores an array of component arrays: `void **`. That is
forbidden, definitively, and the ban is not to be worked around with a typedef.

Storage is therefore **handle- and index-based, struct-of-arrays, one level of
indirection**. This is a better design than the one the rule excluded, not a
compromise: handles survive reallocation, indices are trivially serialisable to
text, and nothing holds a pointer into storage across a frame.

### Other

- **The ECS sits near the bottom of the dependency graph.** Almost everything
  depends on it; it depends on nothing but foundation and maths. This largely
  fixes the shape of the module map before it is drawn.
- Whether the **renderer's own internals** are also ECS is a separate question
  and is *not* decided here — see D-027.
- Archetypes vs sparse sets, and the query API, are implementation and are
  deferred until a card needs them.
- Anything that must be undoable or editable later must be component data.
  State hidden in a system's private variables is invisible to the editor.

## Rejected options and why

- **Option A (node tree)** — rejected because it contradicts `guidelines.md` on
  both counts, and because it makes the text-serialisation requirement of
  ADR-0009 substantially harder for no compensating gain.

## Questions this opens

- **D-027** — are the renderer's internal structures ECS too, or does the
  renderer consume ECS data and keep its own per-frame layout?
- **D-024** — what type description does a component carry so it can be written
  and read back as text?
