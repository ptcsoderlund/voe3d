# physics

What an entity collides as, and the shape that becomes in the world: colliders
read a transform, and `3d` draws them, so collision is neither folder's (0253).

- `include` — the public headers, in `include/physics/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/physics/body_component.h` — a kinematic body's step height, slope
  limit, velocity (wanted in, moved out) and whether it is on the floor, as a
  described field list.
- `include/physics/body_system.h` — the whole-row intent registered as the
  body's replace, the call that creates one, the drain that keeps a row over
  a bad step, slope or velocity, and the move that slides and steps.
- `include/physics/collider_component.h` — a box, sphere or capsule, its size
  in the entity's own units and whether it is a trigger, as a described field
  list with kind names for a dropdown.
- `include/physics/collider_system.h` — the whole-row intent registered as the
  collider's replace, the call that creates one, and the drain that keeps a row
  over a bad size and warns over an unknown kind.
- `include/physics/overlap.h` — the one query: the contacts a sphere or capsule
  overlaps, each with its entity, push-out normal, depth and trigger flag.
- `include/physics/shape.h` — a collider in the world: kind, double centre,
  rotation and half sizes with the transform's scale applied.
