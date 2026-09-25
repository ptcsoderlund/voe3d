# physics

The public headers, one entry each; the fuller account of every one of these
stays on `physics/physics.md`.

- `collider_component.h` — kind, size and trigger, and the reads.
- `collider_system.h` — the intent that replaces one, and the direct call that
  creates one.
- `overlap.h` — what a sphere or capsule overlaps, as push-out contacts.
- `shape.h` — a collider in the world, worked out from its transform.
