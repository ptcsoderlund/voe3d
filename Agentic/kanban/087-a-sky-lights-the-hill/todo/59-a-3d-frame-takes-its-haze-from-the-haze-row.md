# 59 — A 3d frame takes its haze from the Haze row
folder: 3d
after: 56, 57, 58
decisions: 0168, 0416, 0417

## Change
0417 point 1: `voe_3d_draw_system_sky` copies the sky row's `haze` and `haze_start`, which card 57
removed. Read the comment above `voe_3d_draw_system_sky` in `3d/include/3d/draw_system.h`, the
header of `3d/src/draw_system.c`, `scene/include/scene/haze_component.h` (card 56) and the header
of `3d/tests/sky.c`.

- `3d/src/draw_system.c`: `voe_3d_draw_system_sky` fills `drawn`, the colours, horizon height,
  strength and the discs from the first sky row as now, zeroed with none; and `haze` and
  `haze_start` from the first Haze row's `haze` and `start`, found as the sky's table is, 0 with no
  haze table or row. Neither row is the zeroed record. The file's header names the Haze row.
- `3d/include/3d/draw_system.h`: that comment says which fields come from which row and that a
  Haze row without a sky row hazes with `drawn` 0.
- `3d/tests/sky.c`: the rows built drop `haze` and `haze_start`; the world builder can add a haze
  table and row. Record cases: no sky and no haze table, zeroed; a sky row and no haze table,
  `haze` and `haze_start` 0; a haze row of 500 and 250 and no sky table, `drawn` 0, the colours 0,
  `haze` 500, `haze_start` 250; both rows, every field the rows'. The header's list follows.
- `3d/3d.md`, `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: the draw_system and
  sky entries name the Haze row where they name the sky's fields, each one sentence of at most 300
  characters.

## Done when
The tests `3d/sky`, `3d/no_light` and `3d/draw_system` pass after the folder's build.
