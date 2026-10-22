# 0467 — Models blend into the ground in 0.3, before the hill ships
date: 2026-10-22
by: tech-lead

## Decision
Mesh and terrain blending, as in CryEngine 5.6, is part of 0.3 (lumberhill, 0374) as work order 096, taken
before the hill is shipped. A model blends only where it is switched on: per placed thing, by a Blend with
ground switch and a height in the Inspector, and per foliage kind, by the same switch and height in the
Landscape panel. Both are off by default. Grass never blends. The work orders after 095 are renumbered: 096
Models blend into the ground (new), 097 the hill shipped (was 096), 098 snap (was 097), 099 docking panels
(was 098). Earlier decisions that name the old numbers mean the work these are now: 0438 means 098, 0439
means 099, and 0454's shipped hill is 097.

## Reasoning
The sponsor asked for it in lumberhill (2026-10-22). It goes in before shipping so the shipped hill has its
rocks standing in the ground, and after grass and compressed textures so the ground it blends with is
settled. The cost is per pixel inside the blend height, not per copy, so a forest pays only for its trunks'
feet. Grass is all ground-level pixels and would pay for every one of them.
- Next in the queue, before the cooked copy: it would stop the loading work halfway.
- A switch per material: one rock material could not blend on one rock and not on another.
- Always on: a concrete base or a post wants a hard edge.
- Placed models only, no foliage: as cheap to build per kind, and the sponsor wants it.

## Replaces
Nothing. Renumbers 0455's 096–098 as above; removes the idea from `ideas.md`.
