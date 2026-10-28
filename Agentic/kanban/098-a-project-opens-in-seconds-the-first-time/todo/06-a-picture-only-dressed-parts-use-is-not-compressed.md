# 06 — A picture only dressed parts use is not compressed
folder: game
after: 05
decisions: 0168, 0484, 0488, 0420, 0470

## Change
0488. Public surface: `voe_game_models_update_cooked` gains the parts table; its only caller is
`game/src/run.c`, in this folder.

- `game/src/models_batch.h`, `game/src/models_batch.c`:
  - `voe_game_models_batch_new` takes `const voe_game_parts *parts` (NULL for none) after the
    device;
  - before decoding, a file's dressing is the `voe_game_model_parts` row whose path is the file's
    in `parts` when given, else the `.parts` beside the `.glb` (read as the levels' `[Levels]`
    already are), else none;
  - a model image that only glTF materials with a non-empty `material<N>` use (through their
    textures, `assets/model.h`) is not decoded: its picture is a 1×1 white RGBA8 one on its
    arena, compressed by its kind like any other (the write asserts the kind's format);
  - THE PHASES in the header says so in a phrase, naming 0488.
- `game/src/models.c` — update, prepare, cook, watch and reload pass NULL;
  `voe_game_models_update_cooked` passes its new parameter.
- `game/include/game/models.h` — `voe_game_models_update_cooked(world, models, device, folder,
  parts, scratch, progress)`; the COOKED COPIES paragraph: a dressed part's picture is a white
  texel in the copy (0488).
- `game/src/run.c` — its `voe_game_models_update_cooked` calls pass `&voe_game_parts_cooked`.
- `game/src/models_cooked.h` — A MAP'S COPY: a picture both the `.glb` and a map hold is
  compressed once, as the map.
- `game/src/src.md` — the `models_batch.c` entry.

## Done when
`game/tests/models_batch.c` passes with a new check `a_dressed_picture_is_a_white_texel`: a
`.glb` of two materials, each with its own 8×8 picture (`glb_build.h`), its `.parts` dressing
material 0; its copy holds picture 0 at 1×1 and picture 1 at 8×8. With no `.parts` file and the
same dressing given as a `voe_game_parts` row to `voe_game_models_update_cooked`, the copy it
writes does too. Its entry in `game/tests/tests.md` names it. `game/tests/models_cooked.c`'s
`voe_game_models_update_cooked` calls pass an empty table, and it still passes.
