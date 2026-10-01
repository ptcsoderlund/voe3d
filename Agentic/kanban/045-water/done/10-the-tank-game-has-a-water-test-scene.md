# 10 — The tank game has a water test scene
folder: examples
after: 09
decisions: 0168, 0272, 0305

## Change
Data only. 0272: the sponsor builds the levels and agents never edit
`examples/tank_game/main.scene`; a card may keep a test scene of its own.
Read `examples/tank_game/main.scene` for the format,
`3d/include/3d/water_component.h` for the row's name and fields, and
`examples/tank_game/tank_game.md`.

- `examples/tank_game/water.scene`, new: main.scene's ground, camera, sun,
  tank body and spawner, as they are, plus on each side of the 40 m ground
  (x = ±20) a seabed box 12 m wide and 40 long, tilted about Z so it falls
  from just under the ground's edge to about 3 m down at its far edge, and
  a `voe_3d_water` 12 × 40 over it at y −0.1, centred at x = ±26, with the
  defaults.
- `examples/tank_game/tank_game.md`: an entry for `water.scene`.

## Done when
`grep -c '\.voe_3d_water\]' examples/tank_game/water.scene` prints 2, and
`git diff --quiet HEAD -- examples/tank_game/main.scene` exits 0.

The human's, in the editor on examples/tank_game (feature.md How to test):
1. Open `water.scene` or place water over lowered ground in main.scene; it
   moves in the editor views.
2. Turn the sun; the glints move.
3. The shore is clear at the edge and dark further out.
4. Colour and wave size change live in the Inspector, and undo works.
5. A tank's shadow shows on the water.
6. Play with water on both sides; it looks as in the editor and stays smooth.
