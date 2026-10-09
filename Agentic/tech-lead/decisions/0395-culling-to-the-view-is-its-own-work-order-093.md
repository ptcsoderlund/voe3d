# 0395 — Culling to the view is its own work order, 093, before the hill is shipped
date: 2026-10-09
by: tech-lead

## Decision
Work order 093 makes every view skip what it cannot see: the camera pass and the sun's cascades skip
what lies outside them, using the bounding spheres every mesh already has, and the frame breakdown counts
what each view drew and skipped. The hill's finish moves from 093 to 094, and the work orders keep their
order otherwise. This is culling on the CPU. 0375 point 4 stands: indirect drawing, culling on the GPU
and merged meshes still wait until the frame breakdown names draws as the cost.

## Reasoning
An outside review found that the camera pass draws every object whether it is on screen or not, while
point-light shadows already cull by bounding sphere. It costs the same to build alone as folded into
084, 089 or 090, and the sponsor prefers it as a feature of its own (2026-10-09).
- Built inside 084/089/090, whichever came first: same work, less visible, and the sponsor prefers it alone.
- Before 084: it pays off where the forest and grass arrive, but the sponsor placed it at 093.
- Indirect drawing now: 0375 point 4. Nothing has measured draws as the cost yet.

## Replaces
nothing. Renumbers 0392's 093 to 094.
