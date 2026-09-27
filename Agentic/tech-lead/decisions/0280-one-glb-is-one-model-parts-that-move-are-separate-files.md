# 0280 — One .glb is one model; parts that move are separate files
date: 2026-09-27
by: tech-lead

## Decision
A `.glb` may hold any number of objects, and it imports as one model: every object is merged in,
each where it sat in Blender, as 0277 point 2 already does. The file is placed as one thing and
moves as one. A part that must move on its own (a turret, a barrel, a head) is exported as its own
`.glb` and placed as its own thing, then nested with parenting (0271, work order 037). Files with
several objects are neither refused nor split into several things in 0.2.

## Reasoning
It is what 036 already does, it fits the tank (hull, turret and barrel are three files in 037),
and it keeps one way to build a moving assembly. Alternatives: split a file's objects into a tree
of things on import (needs parenting, and a second way to do what 037 does; kept in `ideas.md`);
refuse files with more than one object (blocks ordinary Blender exports for no gain).

## Replaces
nothing. Confirms 0277 point 2 for multi-object files.
