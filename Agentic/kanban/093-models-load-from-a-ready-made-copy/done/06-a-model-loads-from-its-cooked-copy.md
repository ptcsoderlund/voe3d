# 06 — A model loads from its cooked copy
folder: game
after: 01, 05
decisions: 0168, 0470, 0426, 0457

## Change
0470 points 1, 4, 5 and 7, for `.glb` models. Material maps come in card 07. Each load of a `.glb` first
looks for `<folder>/Cache/models/<path>.model`.

- The copy is fresh when `voe_3d_model_cooked_read` takes it and its key equals the stamps of the `.glb`
  and of its `.parts` (0 when there is none).
- A fresh copy is put as read, with no parse, LODs, decode or bake.
- Otherwise the file loads as today. A caller that cooks then gives the decoded prepared to
  `voe_3d_model_cooked_write` and writes the bytes atomically (`voe_platform_file_write`), making the
  folders. A failed write is a line on stderr and still a load.

Who cooks:

| Caller | Copy it takes | Writes a copy |
|---|---|---|
| `update`, `voe_game_models_prepare` | a fresh one | when it is missing or stale; prepare's `remake` always writes |
| watch, reload | a fresh one | never |
| new `voe_game_models_update_cooked` (the game's) | any copy that reads, key not compared | when it is missing |

- New `game/src/models_cooked.h` / `.c`, internal. It holds:
  - the copy's path;
  - the key from the two stamps;
  - read-if-usable, given whether to compare the key: it reads the whole file onto the file's arena, so
    the bytes outlive the put;
  - make-and-write.

  Its header says who cooks, as the table above.
- `game/src/models_batch.h` / `.c` (669 lines; keep it under 800 by putting the logic in
  `models_cooked.c`):
  - `voe_game_models_batch_new` gains the batch's cooking mode, one of the three rows above.
  - A new phase, COOKING, timed and printed in the `game: models:` line after uploading. Reading a copy
    counts as reading.
  - The progress phase is "Preparing models" while the file being loaded is cooked, and "Loading models"
    otherwise. Give the batch the progress if it needs it.
- `game/src/models.c`: update, watch and reload pass their mode. Add `voe_game_models_update_cooked`, the
  same as update with the trusting mode. Prepare also makes the copy: read its header in `models.h` and
  find where it lives in `models.c` or `models_batch.c`. Its `made` is true when either file was made.
- `game/include/game/models.h`:
  - a COOKED COPIES paragraph: the path, freshness, who writes, and that the game trusts;
  - the PHASES and PROGRESS paragraphs name cooking and the two phase strings;
  - `voe_game_models_update_cooked` declared beside update, with the same parameters.
- `game/src/src.md`: the entries for `models_cooked.h`/`.c`, and the batch's mode.
- New test `game/tests/models_cooked.c`. It includes `glb_build.h` and uses a folder of its own, as
  `game/tests/models.c` does. It skips without a graphics card. Its cases:
  - `update_writes_the_copy`
  - `a_fresh_copy_is_read_not_written_again` (the copy's stamp is unchanged)
  - `a_changed_glb_writes_it_again`
  - `watch_reloads_and_writes_no_copy`
  - `update_cooked_loads_a_copy_whose_glb_is_garbage`
  - `prepare_writes_the_copy`

  And its entry in `game/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R '^game/models'` passes, with `game/models_cooked` among them.
