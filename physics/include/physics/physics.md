# physics

The public headers, one entry each; the fuller account of every one of these
stays on `physics/physics.md`.

- `body_component.h` — step height, slope limit, velocity and on-floor, and the
  reads.
- `body_system.h` — the intent that replaces one, and the direct call that
  creates one.
- `collider_component.h` — kind, size and trigger, and the reads.
- `collider_system.h` — the intent that replaces one, and the direct call that
  creates one.
- `overlap.h` — what a sphere or capsule overlaps, as push-out contacts.
- `shape.h` — a collider in the world, worked out from its transform.
- `sweep.h` — solid colliders gathered, and a sphere or ray swept to the first
  one it hits.
