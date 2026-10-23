# 07 — Material maps load from cooked copies, and the game trusts its own
folder: game
after: 06
decisions: 0168, 0470, 0466

## Change
0470 points 2, 4 and 5 for the maps a material names, and the game switched to trusting its copies.

- `game/src/models_cooked.h` / `.c` (card 06): add a map's copy, `<folder>/Cache/pictures/<path>.<colour|normal|data>`.
  - Kind by slot: colour and emission are colour, normal is normal, ORM is data.
  - Its key is the map's stamp.
  - Read-if-usable uses `voe_assets_image_cooked_read`, comparing the key or not. The bytes are read onto
    the map's own arena, so they outlive the put.
  - Make-and-write uses `voe_assets_image_mips` then `voe_assets_image_cooked_write`, written
    atomically. A failed write is a line on stderr.
- `game/src/models_materials.c` (406 lines): each map tries its copy first.
  - A usable copy goes to `voe_3d_models_put_material` with its levels, undecoded.
  - Otherwise the map is read and decoded as today. A caller that cooks then writes the copy, and puts
    the mipped picture.

  Who cooks:

  | Caller | Writes a copy |
  |---|---|
  | `voe_game_models_materials` | when missing or stale |
  | `voe_game_models_material_load` | never |
  | new `voe_game_models_materials_cooked` | trusts any copy that reads; writes one only when missing |

  Cooking is timed as COOKING (card 06) and added to the `game: materials:` line's phases.
- `game/src/models_batch.h`: `VOE_GAME_MATERIALS_LINE` gains COOKING.
- `game/include/game/models.h`:
  - the MATERIALS paragraph names the map copies;
  - `voe_game_models_materials_cooked` declared beside `_materials`, with the same parameters;
  - `voe_game_models_material_load`'s comment says it reads a fresh copy but never writes one.
- `game/src/run.c`: its three `voe_game_models_update` calls become `voe_game_models_update_cooked`, and
  its three `voe_game_models_materials` calls become `voe_game_models_materials_cooked`. The first two
  are near lines 170–175, the rest near lines 268–290. Its header comment, where it names the loader,
  says the game trusts its copies (0470 point 5).
- `game/src/src.md`: the `models_materials.c` and `models_cooked.c` entries, and `run.c` if its entry
  names the loader.
- `game/tests/models_cooked.c` (card 06): add a material with a 16×16 PNG colour map, written with
  `voe_assets_png_encode` or as `game/tests/models.c` makes one, and these cases:
  - `materials_write_the_map_copy`: its file has a full chain (`voe_assets_image_cooked_read`, levels
    equal `voe_assets_image_level_count`).
  - `material_load_writes_no_copy`
  - `materials_cooked_loads_a_copy_whose_png_is_garbage`

  The `game/tests/tests.md` entry names them.

## Done when
`ctest --test-dir build/debug -R '^game/models_cooked$'` passes with the three new cases.
