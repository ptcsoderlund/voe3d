# 30 — A gun fires along its turret's barrel
folder: examples
after: none
decisions: 0168, 0283, 0272

## Change
Bug 03: shells leave out of the back of the barrel. The owner is the gun's `fire` in
`examples/tank_game/Code/tank_gun_system.c`: it spawns along the gun entity's world −Z and puts the
muzzle on that side. The turret on the same entity aims so that its barrel is its −Z turned by
`aim` about its own +Y (`tank_turret.h`); `tank_head` has `aim = 180` because its model faces +Z.
So the turret points the barrel at the pointer and the gun fires the other way. `tank_gun.h`
already says a gun "sits on a turret so it fires along its aim"; the code does not do that. Only
the gun changes. The turret, shell and data files stay as they are.

Files, all under `examples/tank_game/Code/`:

- `tank_gun_system.c`: `fire` works out the barrel rotation: the gun's world rotation
  (`voe_scene_transform_world`) times `voe_math_quat_from_axis_angle` about +Y by the `aim` of the
  entity's `tank_turret` row (`voe_ecs_component_get` with `tank_turret_key`; 0 when the entity has
  no turret row), normalized. This is the same product the turret system builds as `barrel`
  (read `tank_turret_system.c` around its `barrel` for the order of the multiply and the degree
  constant). The muzzle offset is turned by the barrel rotation, and the prefab is spawned with
  it. Include `tank_turret.h`. Header points: the shot's frame is the barrel, i.e. the gun's world
  rotation (every parent's rotation included) turned by its turret's `aim`; with no turret it is
  the gun's own frame; the reason is that a model facing +Z needs `aim`.
- `tank_gun.h`: `muzzle` is in the barrel's frame (the gun's frame turned by the turret's `aim`),
  not the gun's own frame. The shot flies along the barrel.
- `Code.md`: the `tank_gun_system.c` entry says it fires along the turret's barrel.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`grep -q tank_turret_key examples/tank_game/Code/tank_gun_system.c` exits 0. For the human: in the
editor, Play `examples/tank_game` and hold fire. Shells leave the end of the barrel and fly the way
it points. Change the turret's `aim` in the Inspector and the barrel and the shells turn together.
