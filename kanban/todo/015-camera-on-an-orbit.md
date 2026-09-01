# 015 — a camera, on a hardcoded orbit

status: todo
claimed-by: -
blocked-by: 014

## Goal

The camera orbits the scene on its own, with no input. Proves the whole matrix
chain end to end.

## The scene, and it is the point

**Two cubes. One rotating on its own axis, one completely still. The camera orbits
both.**

The principal's design, and it is the only arrangement that is unambiguous: with
one cube you cannot tell an orbiting camera from a rotating cube. Three independent
motions means each one is identifiable, and a mistake in any of view, model or
projection shows up as a specific wrong thing rather than "something looks off".

**This is also where the front-face and Y-flip check gets its human counterpart.**
Card 013 proves it with a pixel readback; a cube you can see all sides of is what a
person can judge.

## Scope

- A view matrix from a position and a target. A projection matrix — and note that
  `math` deliberately builds no projection matrices (ADR-0035), so this belongs to
  whichever folder owns the camera.
- **Per-object model matrices**, since there are now two objects. Push constants or
  a per-draw uniform; say which and why.
- **The rotation type** the principal asked for in the original maths card arrives
  here, in `math`, with tests. This is the card where something finally rotates, so
  implement-on-demand is satisfied.

## Deliberately not the world-as-tables folders

`ecs` and `scene` are **not** written by this card. Two objects is not many
objects, and the tech lead was wrong to say earlier that a camera brings the ECS
with it. Tables arrive when there is something to tabulate — model loading or
later. If this card feels like it needs them, that is a finding to report.

## Verify

- All three motions distinguishable by eye, and the still cube genuinely still.
- Nothing clipped at the near plane when the camera passes close.
- `check.cmake` zero. Windows is the principal's.
