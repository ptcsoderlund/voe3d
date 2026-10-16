# 09 — The loader makes and reads a model's levels file
folder: game
after: 06
decisions: 0168, 0425, 0426

## Change
0426 point 2: the loader the editor and the game share keeps each `.glb`'s levels in
`<folder>/Cache/levels/<model path>.levels`, made when missing or stale, and loads the model with
them. `3d/include/3d/model_levels.h` and `3d/include/3d/models.h` say what the 3d calls do.

- `game/include/game/models.h`:
  - new `[[nodiscard]] bool voe_game_models_prepare(const char *folder, const char *path,
    voe_base_arena *scratch, bool *made, voe_base_error *error)`: reads `<folder>/<path>`, reads
    its levels file, and when that is missing or stale (`voe_3d_model_levels_read` false) makes it
    (`voe_3d_model_levels_make`) and writes it; `made` true when it made one. False, `error` set,
    only when the `.glb` will not read or parse; a write that fails is one line on stderr and true.
    Scratch rewound to where it stood. For the editor's import (0426 point 3).
  - the header gains a LEVELS paragraph: a `.glb` path (any case) loads with its levels file,
    made first when missing or stale, in update and watch alike, so a re-export or a hand-copied
    file is made where it is loaded; the path rule; the write atomic, folders made as needed;
    `Cache/` deletable; a failed write still draws the levels just made. The constraints'
    sentence on scratch says it holds the file, its levels and their making.
- `game/src/models.c`: one static helper shared by `_prepare` and `load_changed`: given the
  folder, path and the `.glb`'s bytes, hands back the levels bytes (read, or made and written) on
  scratch, and whether it made them. `load_changed` calls it for a `.glb` path and loads with
  `voe_3d_models_load_levelled`; other paths load as now. The levels path is
  `voe_platform_path_join` of the folder, "Cache/levels" and `<path>.levels`; before a write each
  folder from `Cache` down to the file's own is made with `voe_platform_folder_create` only when
  absent (`voe_platform_folder_list` on it fails), so nothing is said on stderr for one already
  there; the file written with `voe_platform_file_write`. The header names the levels file. 252
  lines now: if it would pass ~400, put the helper and `_prepare` in a new `game/src/models_levels.c`
  instead, with a header comment, declared in a small internal header.
- `game/src/src.md`: the entry (or entries) names the levels file.
- `game/tests/models.c`: a new case after the existing ones, its own store, with the one-triangle
  `.glb` the file builds:
  1. an update of a world naming `GOOD` leaves `<scratch folder>/Cache/levels/good.glb.levels`,
     which `voe_3d_model_levels_read` takes against the `.glb`'s bytes;
  2. `voe_game_models_prepare` on that path: true, `made` false;
  3. the levels file overwritten with "stale": prepare true, `made` true, the file read again
     fits;
  4. prepare of `MISSING`: false.
  The header's list of cases names it. `game/tests/tests.md`: the `models.c` entry names levels.

## Done when
The test `game/models` passes after the folder's build, with the new case in it.
