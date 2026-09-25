# 0253 — Physics is a folder between scene and 3d, and its first query is overlap
date: 2026-09-25
by: planner

## Decision
For 027 (0249 layers 1 and 2, built on 0250's double positions):
1. **A new code folder `physics`**, row `scene ecs math base` in `cmake/voe.cmake`. `3d` gains
   `physics` (it draws a collider's lines and names the collider that fits a shape), `game` gains
   it (its world holds colliders and bodies and runs the move), `editor` gains it (world step,
   fitting a new collider). No other folder names it.
2. **A collider** is `kind` (box 1, sphere 2, capsule 3), `size` and `trigger`, in the entity's
   own units and scaled by its transform: a box is `size` along its local axes; a sphere's
   diameter is `size.x`; a capsule stands on local Y, diameter `size.x`, whole height `size.y`.
   Default: a box of 1. It needs a transform. The collider that fits a built-in shape: cube and
   cylinder a box of 1, capsule a capsule of (1, 2, 1).
3. **One query in 027: overlap** of a sphere or capsule against every collider, returning
   contacts (entity, push-out normal, depth, trigger). Ray and sweep wait for a caller (rule 10).
   The arithmetic runs in float relative to the query shape's centre (0250).
4. **The kinematic body moves in substeps and pushes out of what it overlaps**, no sweep: no
   substep moves further than a quarter of the capsule's radius, so nothing is tunnelled at a
   game's speeds. It holds `step_height` (0.3 m), `slope_limit` (0.8 rad), a read-only
   `velocity` (in: wanted; out: what it really moved) and read-only `on_floor`. The project
   writes the wanted velocity through the whole-row replace intent; the move writes the rest.
   A floor contact pushes straight up, so a body standing on a ramp does not creep down it.
5. **A trigger is noticed by asking**: a project overlaps the trigger's own shape and looks at
   what it touches. The body passes through triggers.

## Reasoning
`scene` must not know about collision and `3d` must draw it, so the folder sits between them;
putting colliders into `scene` would mix what things are with how they collide, and into `3d`
would make a game's logic link the renderer's folder for its maths. Overlap with push-out is
the whole of what a capsule controller needs and is far less arithmetic than an exact capsule
sweep against a rotated box; the substep bound keeps it from missing a wall. Asking a trigger
keeps queries read-only (0249 rule 1) with no event list to own.

## Replaces
nothing. Names the folder 0249 needs.
