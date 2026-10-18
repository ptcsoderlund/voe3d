# 0444 — The engine sets no size limit on what a dev loads; its room grows to the card's memory
date: 2026-10-18
by: tech-lead

## Decision
The engine does not decide how big a model, a scene or a project may be. A dev may build an app that
is one 20-million-vertex model. The room for model geometry on the graphics card (`vertices` and
`indices` in the render capacities) is a starting size, not a limit. When creating geometry finds no
room, the pool grows (a larger pool, the old one copied across and bound in its place) while the card
is idle, which is when geometry is created already. The only limit left is what the machine actually
has. When that limit is hit (the card or the computer runs out of memory), it is reported as an error
in 0443's terms: the file, the numbers, and that the machine has run out. The engine gives no
warnings or advice about size ("heavy for a forest") before a limit is hit. A model's LODs reuse LOD 1's
vertices and add only their own indices, rather than uploading compacted copies, so a model with
LODs needs about half the room. This reverses `render`'s "nothing here grows" for geometry, and for
geometry only. Other fixed capacities stay until a bug shows them in the way, and are then decided the
same way.

## Reasoning
The human: "I don't want to dictate what devs are allowed to do. We don't set limits, we report errors
when limits are hit." Bug 01's fir tree (5.4 M vertices, about 10.7 M with its LODs) didn't fit the
fixed 3 M vertices, though the machine could hold it.
Alternatives: a larger fixed room (16 M vertices) reserves about 830 MB in every program, even one with a cube, and is still a
limit; keep 3 M and report it (the tree doesn't import, against bug 01 and 0280/0376); a size warning on
import (the engine telling the dev what is reasonable, which the human declined).

## Replaces
nothing. Amends 0443: the example limits there stay examples. A limit the engine keeps on purpose
should now be rare, and a lack of memory is reported as the machine's limit.
