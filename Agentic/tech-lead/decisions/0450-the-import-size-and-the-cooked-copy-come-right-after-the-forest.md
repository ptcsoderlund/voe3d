# 0450 — The import size and the cooked copy come right after the forest
date: 2026-10-20
by: tech-lead

## Decision
The work orders are renumbered so that /drive takes them in this order once 090 (the forest) is accepted:
091 Import textures asks how large (new, 0448), 092 Models load from a ready-made copy (new, 0449), 093 grass
(was 091), 094 compressed textures (was 092), 095 culling (was 093), 096 the hill shipped (was 094), 097 snap
(was 095), 098 docking panels (was 096). Earlier decisions that name the old numbers mean the work these are
now; 0438 means 097 and 0439 means 098.

## Reasoning
The sponsor put the cache before the forest (2026-10-20). The forest was already built, so the cache comes
right after it, before the grass multiplies what loads. The import size goes first because it is small and
settles how large the pictures the cache stores are. /drive takes the lowest number, so reordering means
renumbering, as 0377, 0392 and 0395 did.

## Replaces
Renumbers 0395's 093 and 094, 0438's 095, 0439's 096 and 0376's later work orders, as above.
