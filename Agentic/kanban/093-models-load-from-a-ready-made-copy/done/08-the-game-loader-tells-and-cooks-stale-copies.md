# 08 — The game loader tells and cooks stale copies
folder: game
after: 07
decisions: 0168, 0470

## Change
0470 point 6. Before Play and Ship the editor needs two things: a cheap count of stale copies, and a cook
of those that needs no device. Both live here, beside the loader that writes the copies.

- `game/include/game/models.h`:
  - `unsigned voe_game_models_stale(const char *folder, const char *const *paths, uint32_t path_count, const voe_game_materials *table, voe_base_arena *scratch)`.
    It counts how many of the `.glb` `paths` (under `folder`, as update takes them) and how many maps of
    `table`'s rows have a copy that is missing or whose key differs from the input's stamps. It reads only
    each copy's head: `voe_platform_file_read_head` with `VOE_3D_MODEL_COOKED_HEAD` or
    `VOE_ASSETS_IMAGE_COOKED_HEAD` bytes, then `voe_3d_model_cooked_key_of` or
    `voe_assets_image_cooked_key`. An input with no stamp is not counted. A map named twice is counted once.
  - `[[nodiscard]] bool voe_game_models_cook(const char *folder, const char *const *paths, uint32_t path_count, const voe_game_materials *table, voe_base_arena *scratch, voe_game_progress *progress)`.
    It makes and writes every copy `_stale` would count, one file at a time, on the caller, with no
    device, using card 06's and 07's make-and-write. The progress reads "Preparing models" done/total
    over that count.
    - A `.glb` goes through the same steps as `voe_game_models_prepare`, so its levels file is made when
      missing.
    - A file that will not read or parse is a line at its cause and is skipped.
    - Scratch is rewound.
    - It returns false only once the progress says stop.
  - The COOKED COPIES paragraph names the two calls and that they are the editor's before Play and Ship.
- `game/src/models_cooked.c` (or a new `game/src/models_stale.c` if `models_cooked.c` would pass about
  600 lines): the two functions, built on the helpers cards 06 and 07 added.
- `game/src/src.md`: the entry for whichever file holds them.
- `game/tests/models_cooked.c`: add these cases, each on the test's `.glb` and material:
  - `stale_counts_missing_copies`: 2 before any load.
  - `cook_makes_them_and_stale_is_then_0`.
  - `stale_counts_a_changed_glb`: rewritten with another size, 1.
  - `cook_stops_when_asked`: a progress with stop set makes nothing and is false.

  The `game/tests/tests.md` entry names them.

## Done when
`ctest --test-dir build/debug -R '^game/models_cooked$'` passes with the four new cases.
