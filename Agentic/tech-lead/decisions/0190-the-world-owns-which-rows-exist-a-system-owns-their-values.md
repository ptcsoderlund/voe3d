# 0190 — The world owns which rows exist; a system owns their values
date: 2026-09-19
by: tech-lead

## Decision
A world holds entities, and an entity owns its component rows: destroying an entity removes every row it has.
Which rows exist is the world's business. `ecs` keeps one structural queue per world with three requests: add a
row to an entity with given bytes, remove a row from an entity, destroy an entity. Anyone may submit to it. The
world applies the queue at one point in the frame, before the systems run, so no system sees a row appear or
vanish partway through its run. The values in a row still belong to that row's system, and anyone else changes
them through its intent (rule 4, ADR-0134). Every described component type registers a default row in `ecs`
beside its replace intent, set by the folder that declares the type (`voe_ecs_component_default_set`, the way
ADR-0134 registers the intent). "Add at default" means adding a row with those bytes. A system that derives rows
from another row drops them when the source row is gone. For example, `3d`'s shape system drops a shape's
runtime mesh and material. Rule 3 of 0168 now reads: *a row's values are written only by its own system; rows
are added and removed through the world's structural queue; the creation exceptions (a folder's typed creation
call, `authoring`'s reader, ADR-0152) stand as they are.* The editor's Add component, Remove and Delete in 010
use this queue, and so will undo and game logic's spawn and despawn.

## Reasoning
Two things already on the road need to add rows with values other than the defaults and to remove rows at run
time. Undo (milestone 1 of 0186) puts back a removed component with its old values. Collecting a coin (milestones
4–5) despawns an entity while the game runs. One rule that covers the editor, undo and game code is cheaper than
an exception now and a rewrite later. The sponsor's view, adopted: the world is the partition that holds
entities, and entities own their components. Alternatives: an editor-only exception in the shape of ADR-0152,
which allows only adding at defaults and would need a second exception for undo; add and remove through every
owning system's intents, which grows every component folder for one need.

## Replaces
Amends rule 3 of 0168 and widens ADR-0152 point 2 ("never removes a row"), which now holds only for the reader.
