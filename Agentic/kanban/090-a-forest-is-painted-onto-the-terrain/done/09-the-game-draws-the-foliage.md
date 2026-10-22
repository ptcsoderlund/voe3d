# 09 — The game draws the foliage
folder: game
after: 08
decisions: 0168, 0442

## Change
0442 point 5 in the shipped game and Play: each frame gathers the foliage before its shadows,
and the device has room for it.

- `game/src/frame.c` — in `voe_game_frame`, after `frame.models` is set and before the
  blockers, `voe_3d_draw_system_foliage(world, device, &frame, scratch)`, its result ignored
  as the blockers' is (false draws no foliage, no failed frame), placed so the groups on
  `scratch` are not rewound before `voe_3d_draw_system_run` has drawn (take the mark after
  it, or rewind past it only after the pass). A comment says why it is before the shadows.
- `game/include/game/frame.h` — `VOE_GAME_CAPACITIES` gains `.instances =
  VOE_3D_FOLIAGE_INSTANCES`, and its objects VOE_3D_FOLIAGE_GROUPS for the window pass and
  each cascade of each of VOE_RENDER_DIRECTIONAL_LIGHTS lights (foliage draws in no other
  pass). The comment above it says both, and that foliage instances are written once a frame.

## Done when
`grep -n "voe_3d_draw_system_foliage" game/src/frame.c` and `grep -n "FOLIAGE_INSTANCES"
game/include/game/frame.h` each print a line, and the folder's tests pass.
