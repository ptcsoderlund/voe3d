# 24 — What is off collides with nothing
folder: physics
after: 19
decisions: 0168, 0427, 0428

## Change
0428 point 5: `voe_physics_shape_of` is the one gate. Overlaps
(`physics/src/overlap.c`), the obstacle gather sweeps use
(`physics/src/sweep.c`), a body's move (`physics/src/body_move.c`) and every
game's own queries already pass by an entity with no shape, so they need no
change.

`physics/src/shape.c`: `voe_physics_shape_of` returns false, `*out` left
alone, when `voe_scene_identity_off(world, entity)`
(`scene/identity_component.h`) is true, checked before the collider is
read. The file's header comment says so.

`physics/include/physics/shape.h`: `voe_physics_shape_of`'s comment adds
"or is switched off, itself or through a parent (0428)" to when it is
false; the header comment's first paragraph says a switched-off entity has
no shape and so is met by no query, an off body is not moved.
`physics/physics.md` and `physics/include/physics/physics.md` entries for
`shape.h` change only if they list when there is no shape.

Tests, each world also registering the identity table
(`scene/identity_system.h`) and, where a parent is used, the parent table
(`scene/parent_system.h`), the entities they switch given identities:
- `physics/tests/collider.c`: new case `an_off_collider_has_no_shape`: a box
  with a transform has a shape; its identity off: none; on again: the same
  shape; a child box under an off parent: none.
- `physics/tests/overlap.c`: new case `an_off_floor_is_not_touched`: the
  capsule on a floor as the first case sets it, the floor off: no contact.
- `physics/tests/sweep.c`: new case `an_off_sphere_is_not_gathered`: two
  spheres, one off: the gather writes the other only.
- `physics/tests/body.c`: new case `a_body_falls_through_an_off_floor`: a
  body over a floor made by `floor_to`, walked in place by `walk` (the
  test's own gravity), the floor off: it ends below the floor's top; and an
  off body walked: its transform does not change.
Each in its file's `main` and header; `physics/tests/tests.md` entries gain
the switch, each one sentence under 300 characters.

## Done when
The tests `physics/collider`, `physics/overlap`, `physics/sweep` and
`physics/body` pass after the folder's build, each with its new case.
