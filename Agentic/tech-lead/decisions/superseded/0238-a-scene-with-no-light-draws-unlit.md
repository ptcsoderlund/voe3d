# 0238 — A scene with no light draws unlit
date: 2026-09-24
by: tech-lead

## Decision
A scene does not need a light. Drawing a frame of a scene that has none never asserts or fails, not in the
editor's views and not in the game that Play starts. With no light, every surface draws in its own material
colour, unshaded. Adding a light brings shading back. A 2D game, or any game that wants everything lit as it is,
builds by just adding no light. This decision says nothing about more than one light; that limit stays as it is
for now (see `ideas.md`). The camera is unchanged: exactly one per scene (0218).

## Reasoning
A 2D game, or one that treats everything as already lit, has no light to add, and the engine must not force one
on it. Unshaded is the cheapest and most predictable meaning of "no light", and it is exactly the 2D case.
Rejected alternatives:
- ambient only: a lightless scene would be dim, so a 2D game's colours would come out wrong.
- a hidden default light: a light the developer never added would still shade their surfaces.

## Replaces
nothing
