# 46 — The loader loads one model again with its levels
folder: game
after: none
decisions: 0168, 0426, 0431

## Change
0431 point 2: Make levels rewrites a `.glb`'s levels file, but the store keeps the levels it loaded,
since the watch reloads only on a changed `.glb` stamp. The loader gains a call that loads one held
model again, stamp or not. The editor's call of it is card 47's; leave `editor/`.

- `game/include/game/models.h`: declare, after `voe_game_models_prepare`,
  `[[nodiscard]] bool voe_game_models_reload(voe_3d_models *models, voe_render_device *device,
  const char *folder, const char *path, voe_base_arena *scratch, voe_base_error *error)`. Its comment:
  a path the store holds (`voe_3d_models_find`) is read and loaded again with its levels file
  whatever its stamp, at the file's stamp; a path it lacks is nothing and true; false, `error` set,
  when the load failed, the entry kept as the store keeps a failed reload; between frames; call the
  materials call after, as after a watch; for the editor's prepare (0431). The why goes above the
  declaration only: the header comment is at its cap, add nothing to it.
- `game/src/models.c`: the function. Reuse `load_changed` (line ~208), which already reads a path,
  stamps it and loads a `.glb` with `levels_for`'s bytes: give the reload a way past its `known`
  stamp test, either by moving the read-and-load after that test into a static of its own that both
  call, or by a `bool` on `load_changed`; whichever, its comment stays true. Scratch rewound to where
  it stood. The file header gains a phrase: one held path loaded again on asking. 421 lines now: if
  it would pass ~450, move `levels_asked`, `levels_for`, `voe_game_models_prepare` and the reload's
  static into a new `game/src/models_levels.c` with its header comment, declared in a small internal
  header `game/src/models_levels.h`, and give both a `game/src/src.md` entry.
- `game/tests/models.c`: a new case after `check_levels_asked`, called from `main` where
  `check_levels` is, on its own store and world as `check_levels` makes them, with `GRID` written in
  the scratch folder as `check_levels_asked` writes it:
  1. no `grid.parts`, a world naming `GRID` (`wear`) and an update: the entry's `level_count` is 3
     or more;
  2. `grid.parts` written with `[Levels]` `count=2` and a prepare with remake: the entry's
     `level_count` is unchanged (the store is not touched by a prepare);
  3. `voe_game_models_reload` of `GRID`: true, and the entry's `level_count` is 2;
  4. `voe_game_models_reload` of a path the store lacks: true, `voe_3d_models_count` unchanged.
  The header's THE LEVELS paragraph names the reload in a phrase, staying within its cap of 60.

## Done when
The test `game/models` passes after the folder's build, with the reload case in it.
