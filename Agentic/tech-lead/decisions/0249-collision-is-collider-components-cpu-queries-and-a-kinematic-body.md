# 0249 — Collision is collider components, CPU queries and a kinematic body, built in layers
date: 2026-09-25
by: tech-lead

## Decision
Physics comes in three layers, each on the one below, and a later layer is added beside the
earlier ones and never replaces them:
1. **Colliders and queries.** Box, sphere and capsule colliders are components on entities.
   The engine answers ray, sweep and overlap queries against them on the CPU. A collider can be
   a **trigger**, which blocks nothing and reports what touches it.
2. **Kinematic body.** A component with a move-and-slide: a system gives it a wanted velocity,
   it slides along what it hits, steps over low edges, holds to a slope limit, and reports
   whether it is on the floor. **Gravity and jumping are the project's own code** (0239), not
   the engine's.
3. **Dynamic bodies** (things pushed by forces, stacking, tumbling). Not on the road for the
   coin game (0186); an idea until a game needs it.

Five rules hold for every layer:
1. **A query only reads.** Ray, sweep and overlap read collider components and change nothing,
   no hidden cache a query updates.
2. **A body's move is a transform intent** (ADR-0011, ADR-0017), never a direct write.
3. **Physics is ADR-0065's fixed-step phase**: an accumulator with a fixed step and a maximum
   number of steps per frame, and the draw interpolates between the last two states. The
   planner sets the numbers (D-082) and the transform module owns the previous state (D-083).
4. **Colliders are plain component data**, so another reader, such as GPU particles, can read
   the same shapes.
5. **One thread now** (ADR-0065 rule 6). Rules 1 and 2 are what keep a later threading
   decision open: queries split across cores with no locks, and moves drain in one place.

Gameplay collision stays on the CPU. **GPU compute is for particles**: they collide one way
against an uploaded copy of the colliders (or the depth buffer), and game logic never reads a
result back from the GPU.

## Reasoning
A character driven by forces slides, bounces and gets pushed; players expect instant stops,
steps and slopes, which is why Godot's `CharacterBody3D` and PhysX's capsule controller are
kinematic bodies in the physics engine with move-and-slide on top. Gameplay code runs on the
CPU and needs its answer in the same frame; a GPU answer arrives late or stalls the frame, and
no major engine runs gameplay physics on the GPU. Particle systems (Niagara, VFX Graph) do run
on the GPU, one way, which rule 4 keeps possible. Jolt's shape (read-only queries, results
applied afterward, islands for the solver) is the model for threading later. Rejected: a
dynamic rigid body as the character (feels bad, must be fought); queries only, with each game
writing its own controller (every game repeats the tricky part); all physics on GPU compute
(readback latency for game logic, slower at small counts, harder to debug).

## Replaces
nothing. It fills in milestone 4 of 0186 and answers the `character_controller` idea.
