# 0428 — The switch is the identity's `off`, and one read answers for every system
date: 2026-10-15
by: planner

## Decision
For 089 bug 01, carrying out 0427.
1. The switch is a `bool off` on `voe_scene_identity`, after `folded`. False is on, so every identity made in
   code and every older file without the key is on. It is saved, undone, copied and cooked with the identity,
   as `folded` is (0302): no table for one bit. Only authored entities (those with an identity) have it.
2. One read, `voe_scene_identity_off(world, entity)`: true when the entity's own `off` or any ancestor's is set,
   walking parents up to `VOE_SCENE_PARENT_DEPTH_MAX` links; false for a dead entity or a world without the
   identity table. Every system asks it of a row's owner; none keeps a copy.
3. Draws, shadows, bounce, water and particles leave an off owner out where they leave out the frame's
   `hidden`, through one 3d helper. An off emitter's particles are not drawn; its births go on.
4. An off directional light keeps its row, so the order of sky discs, `more_lights` and shadow slots stays as
   the table has it; it is handed over with intensity and fill nought and does not cast. With every sun off the
   scene is dark, not lit by a fallback. Off point lights and light blockers are skipped.
5. `voe_physics_shape_of` is false for an off entity, so overlaps, sweeps, a body's move and a game's own
   queries all pass it by, and its collider outline is not drawn.
6. An off sound's voice is stopped and not started; its `playing` is left as it is, so switching on resumes it.
7. Picks and the editor's markers pass an off entity by. A selected off entity keeps its outline, bounds and
   gizmo, so a person sees what the list selected.
8. The camera's entity shows no switch (one camera, ADR-0218), and cameras ignore `off`.
9. The Inspector's top row flips `off` through the identity intent, one structural change, as a fold is. The
   identity section lists `off` as it lists `folded`. The Scene list draws an off row under its dimmed theme.

## Reasoning
`folded` already proved one bit on the identity saves, undoes and copies with no new table and no new file
key handling. A read that walks ancestors keeps children's own switches intact (0427) and puts the rule in one
place; a cached "effectively off" flag would be state to keep in step with every parent change. Zeroing an off
sun instead of dropping its row keeps every per-row array in table order. Making `shape_of` the one gate in
physics covers the engine's queries and every game's in one change.

## Replaces
nothing
