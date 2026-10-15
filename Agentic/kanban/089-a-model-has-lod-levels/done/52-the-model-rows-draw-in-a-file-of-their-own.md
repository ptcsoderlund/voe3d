# 52 — The model rows are drawn from a file of their own
folder: 3d
after: 51
decisions: 0168

## Change
`3d/src/draw_system.c` is 789 lines and card 53 adds to its model-row walk. Move that walk out
first, unchanged in behaviour.

- `3d/src/draw_models.c` (new): `draw_models` (line ~398) as `voe_3d_draw_models`, with the statics
  only it uses (`drawn_model`, `model_part_entry`, and any other the coder finds only it calls). A
  static that `draw_system.c` still needs too (`is_landscape`, `model_part_count`, if so) moves here
  and is declared in the header, not copied. Header comment: what the walk draws and holds (the
  sentences on model rows, fade and level from `draw_system.c`'s header move here).
- `3d/src/draw_models.h` (new): the declarations, with a short header comment; internal to 3d,
  shaped as `draw_terrain.h`.
- `3d/src/draw_system.c`: includes `draw_models.h`, calls `voe_3d_draw_models` where it called
  `draw_models`; its header keeps one phrase pointing at `draw_models.c` for model rows.
- `3d/src/src.md`: entries for `draw_models.h` and `draw_models.c`, one sentence each; the
  `draw_system.c` entry no longer claims what moved.

No test changes: the existing ones are the proof.

## Done when
`wc -l 3d/src/draw_system.c` prints under 720, and the tests `3d/draw_system`, `3d/draw_levels`
and `3d/draw_model_fade` pass after the folder's build.
