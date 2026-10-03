# 0335 — Every entity in the editor has an identity
date: 2026-10-03
by: tech-lead

## Decision
Every entity the editor makes or holds has an identity component, always: Add entity, Duplicate, a
placed prefab, a spawned copy, an entity read from a scene file and one brought back by undo. The
Inspector shows the identity section on every entity, and it has no Remove. A scene file row without an
identity gets one when it is read into the editor. This holds however bare the entity is otherwise
(0300): bare means no transform, never no identity.

## Reasoning
The sponsor's standing call (2026-10-03, "We always want identity component for editor entities").
The editor names, selects, undoes and saves entities by their identity (0193, 0204), so an entity
without one cannot be edited properly. 0300 already said a new entity keeps "its editor-managed name";
this says it for every path, so no later work drops it again.
- Allow identity-less entities and show them as "(unnamed)": a second kind of entity every panel must
  handle.
- Add the identity only when it is first renamed: the Scene list, undo and selection would have nothing
  to hold on to until then.

## Replaces
Nothing. Sharpens 0300 (bare means no transform, not no identity) and 0193.
