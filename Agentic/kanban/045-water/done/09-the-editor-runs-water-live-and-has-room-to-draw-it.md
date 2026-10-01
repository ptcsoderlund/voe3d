# 09 — The editor runs water live and has room to draw it
folder: editor
after: 08
decisions: 0168, 0305

## Change
0305 point 5. Read `3d/include/3d/water_system.h`,
`game/include/game/world.h` (`VOE_GAME_WORLD_WATERS`),
`editor/src/world_step.h`, `editor/src/world_step.c` and
`editor/src/view_passes.h`.

- `editor/src/world_step.c`: `voe_3d_water_system_run(world, seconds)`
  after the emitters' run, so water moves in edit and play alike; the
  header's first line names it.
- `editor/src/view_passes.h`: the capacities' objects gain
  `VOE_GAME_WORLD_WATERS` in every view's pass and the preview's, and the
  comment says water casts no shadow and each target now costs two texture
  slots (card 02).
- `editor/src/src.md`: the world_step entry if its line changes.

The Inspector, Add component, undo and save are generic over described
components; nothing else changes.

## Done when
`grep -q voe_3d_water_system_run editor/src/world_step.c` and
`grep -q VOE_GAME_WORLD_WATERS editor/src/view_passes.h` exit 0, and the
editor builds in the folder's checks.
