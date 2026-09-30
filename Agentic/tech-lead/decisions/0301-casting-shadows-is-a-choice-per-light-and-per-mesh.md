# 0301 — Casting shadows is a choice per light and per mesh, and point-light shadows are batched
date: 2026-09-30
by: tech-lead

## Decision
Every light type that can cast shadows has a **Cast shadows** option, and so does every drawn mesh (shape and
model). A mesh with it off is still lit and still receives shadows, but casts none. The directional light
and meshes get the option first, in work order 049. Point lights (work order 048) come without shadows and
without the option, and a point light gets it only when point-light shadows are built, in work order 050.
Those shadows are **drawn in batches**: many shadow-casting lights share a few draw calls, not one extra
pass per light, and a limit decides how many lights cast shadows at once. Work orders were renumbered on
2026-09-30: "043's point lights" in 0288 now means work order 048, and 043 is entities without a transform (0300).

## Reasoning
The sponsor's call (2026-09-30): some meshes should not cast shadows even when their light does, and a
separate pass per light is too expensive with a hundred lamps. A point light's shadow is six views of
the scene, so it is a feature of its own, not a checkbox added to 048. Batching may need an instanced or
multi-view draw that `render` does not have today, and `render/vulkan` is the human's to change, not the agents'.
Rejected: point-light shadows inside 048 (a much longer test); a checkbox on point lights that does nothing
until then (misleading).

## Replaces
Nothing. Amends 0252 (all onto all): a mesh may opt out of casting.
