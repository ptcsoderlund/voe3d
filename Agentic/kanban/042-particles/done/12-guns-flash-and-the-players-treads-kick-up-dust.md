# 12 — Guns flash and the player's treads kick up dust
folder: examples
after: 07, 10, 11
decisions: 0168, 0298, 0299

## Change
0299 points 2 and 3. Read `examples/tank_game/Code/Code.md`,
`tank_gun.h`, `tank_gun_system.c`, `tank_enemy_system.c`, `tank_hull.h`,
`tank_hull_system.c`, `tank_lives_system.c` (how a row is added at run time)
and `3d/include/3d/emitter_component.h`. Never edit `main.scene` or
`tank_body.prefab`.

- `tank_gun_system.c`: a gun whose entity has no emitter gets the muzzle
  flash of 0299 point 2 (the same numbers as `enemy_tank.prefab`'s, offset
  the gun's `muzzle`, direction along its barrel), added the way the lives
  system adds its collider. Each shot fired sends
  `voe_3d_emitter_control_submit` BURST, count 0, for that entity.
- `tank_enemy_system.c`: each shot an enemy fires sends the same burst for
  its turret's entity; an enemy with no emitter there sends nothing.
- `tank_hull_system.c`: a `tank_hull` whose entity has no emitter gets the
  dust of 0299 point 3 (the same look as `enemy_tank.prefab`'s dust, but
  playing false); each step it sends PLAY when the control row's drive goes
  past 0.1 in size and STOP when it falls back, only on the change, read
  from the emitter row's `playing`.
- The headers `tank_gun.h`, `tank_enemy.h`, `tank_hull.h`: one point each on
  the effect their system adds or fires. `Code.md`: the three entries.

The human, after the build: open the tank game and press Play.
1. Standing still, no dust; driving, dust trails behind; stopping, it dies away.
2. Each shot flashes at the barrel, the player's and the enemies'.
3. A hit bursts into fire, then smoke that rises and fades.
4. Blow up ten things in quick succession: the game stays smooth.
5. Stop, select the sun, set it low and orange, then Play: the smoke is
   lit orange on one side. Undo the sun afterwards.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`grep -q voe_3d_emitter_control_submit examples/tank_game/Code/tank_gun_system.c && grep -q
voe_3d_emitter_control_submit examples/tank_game/Code/tank_hull_system.c && grep -q
voe_3d_emitter_control_submit examples/tank_game/Code/tank_enemy_system.c` exits 0. The rest
is the human's steps above.
