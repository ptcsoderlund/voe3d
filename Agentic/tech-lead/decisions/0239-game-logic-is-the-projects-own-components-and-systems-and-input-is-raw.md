# 0239 — Game logic is the project's own components and systems, and input is served raw
date: 2026-09-24
by: tech-lead

## Decision
A project's game logic (0187) is written the way the engine is: the developer defines **components
and systems of their own** in the project's C, and a system iterates the entities that have a given
set of component types. **The engine has no named actions and no input framework.** It serves the
keyboard, mouse and the rest through its public API, and the developer writes their own input
logic. For example, a project defines a `keyboard_input` component and a `keyboard_system` that
reads the keys and fills it in, and other systems react to entities that carry it. Adding or
removing a component on an entity is how a developer turns a behaviour on or off. Milestone 3 of
0186 reads "input is read through the engine's API" in place of "input is read as named actions".

## Reasoning
One way of writing logic, the engine's own, means nothing to learn twice and nothing to throw away
later. An action layer is a framework every game pays for and many would replace. Rejected: named
actions with bindings (the old line in 0186), a single per-frame update function, an embedded
scripting language (already rejected in 0187).

## Replaces
The words "input is read as named actions" in milestone 3 of 0186.
