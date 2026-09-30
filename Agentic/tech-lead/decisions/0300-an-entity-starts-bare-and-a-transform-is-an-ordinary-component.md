# 0300 — An entity starts bare and a transform is an ordinary component
date: 2026-09-30
by: tech-lead

## Decision
**Add entity** makes an entity with only its editor-managed name: no transform. The transform is an
ordinary component: Add component offers it, and the Inspector lets it be removed when nothing else
on the entity needs it. Adding a component that needs a transform (shape, model, camera, light,
emitter, collider…) brings one at the world origin when the entity has none. An entity without a
transform is data only, such as game state a project's systems read and write; it is listed in the
Scene list and appears in no scene view. It can have children and be a child, like any other
entity: a child with a transform under a parent without one is placed in the world as if it had no
parent. In the Scene list, any entity with children can be collapsed and expanded, and it stays
that way when the project is opened again. This is how things are grouped, such as a hundred lights
under one "Lamps" entity.

## Reasoning
The sponsor's call (2026-09-30): logic is written as ECS (0187, 0239), not a separate view-model
layer, so game state is entities and components too, and a game-state entity has no place in the world.
It follows 0222: only things in 3D space are placed by a transform. A transform-less parent also serves as
the group that `ideas.md`'s "entity groups" line asked for, so no editor-only folder type is needed.
Rejected: a new entity still gets a transform, but one you can remove (keeps the one-click case but
contradicts 0222 on every data entity); a second "Add empty" menu item (two ways to do one thing);
editor-only folders (a second kind of tree beside parenting).

## Replaces
Nothing whole. Amends 0217: Add entity makes a bare entity, and the transform can be removed. Amends
0221: the Inspector no longer holds the transform as kept.
