# 0193 — The editor changes structure through the queue and names what it adds
date: 2026-09-19
by: planner

## Decision
For spec 010.
- **Every add, duplicate, delete, Add component and Remove is a structural request** (0190). The editor
  creates the entity itself with `voe_ecs_entity_create`, and every row it gives the entity goes through
  `voe_ecs_structure_add`. It never uses a typed creation call for these. The editor applies the queue once a
  frame, before the systems run. Each request marks the project unsaved through `voe_editor_session_edited`.
- **A new entity's id is one more than the largest identity id in the world. Its name** is the base name
  ("Entity", "Cube", "Capsule", "Cylinder", or a duplicate's source name less a trailing " <number>") if no
  identity has it, else "<base> N" for the lowest N from 2 that is free.
- **Duplicate copies every described row** the source has and gives the copy its own id and name. The shape
  system derives the rest again.
- **The Inspector shows described components only.** Runtime-only rows are the engine's own (a mesh, a
  material) and are neither listed nor removable. A section's heading, and each Add component entry, is the
  key name's last `_` word with its first letter in capitals (`voe_scene_transform` → "Transform"). The identity
  section has no Remove button.
- **What a type needs is registered in `ecs`**: `voe_ecs_component_needs_set(world, type, needed)` says that
  rows of `type` do nothing without a row of `needed`. `3d` registers that a shape needs a transform. A section
  whose type needs a type the entity lacks says "Needs <Heading>". The Inspector still names no component.

## Reasoning
0190 makes the queue the one door for structure, and one path for all five commands is less code than a typed
call for shortcuts and the queue for the rest. An id that is one more than the largest cannot collide within
the file. Rule 10 rules out an id allocator in `scene` while the editor is its only user. Hiding runtime-only
rows keeps a person from removing a mesh the shape system would rebuild the next frame. A "needs" registered by
the declaring folder keeps ADR-0134's promise that the Inspector would show a component written tomorrow
without being touched. Rejected: naming shape and transform in the Inspector, and a general dependency graph
(one needed type is the only case).

## Replaces
nothing
