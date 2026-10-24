# 10 — The materials calls show progress and queue what they cannot wait for
folder: game
after: 09
decisions: 0168, 0370, 0470, 0477

## Change
0477 point 3 in the material loader. Under a splash, the maps' compression shows as progress. Given
a queue, a map with no fresh copy draws uncompressed and waits in the queue. The editor's calls are
card 11's: this card leaves `editor/` alone.

- `game/include/game/models.h`:
  - `voe_game_models_materials(…, voe_game_progress *progress, voe_game_compressing *compressing)`;
  - `voe_game_models_materials_cooked(…, voe_game_progress *progress)`;
  - `voe_game_models_material_load(…, voe_game_compressing *compressing)`.

  What they do, in the header and the function comments:
  - A map decoded because it has no copy to take, with no queue: the writing modes cook its copy,
    compressed (card 08), as today. The one-material load uploads the decoded picture, mipped on the
    GPU, as today.
  - With a queue: the map is mipped on the CPU (`voe_assets_image_mips`) and uploaded RGBA8 with
    those levels. It is added to the queue with its stamp, and no copy is written now.
  - Progress, NULL for none: before each material, "Compressing textures" done/total over the call's
    materials when it has a map to cook inline, else "Loading materials". When a stop is asked, the
    call returns before the next material.
- `game/src/models_materials.c` — the above.
- `game/src/run.c` — the start's call passes its progress; the loop's calls pass NULL.
- `game/tests/models.c`, `game/tests/models_cooked.c` — the callers pass NULL. New cases in
  `models_cooked.c`:
  - with a queue, the materials call loads a map with no copy: its texture is not compressed
    (`voe_render_texture_memory_smooth`), one job waits, and no copy file exists;
  - the queue stepped to its end writes the copy, and the material loaded again takes it,
    compressed;
  - with a progress, the phase reads "Compressing textures" after the call.
- `game/src/src.md`, `game/tests/tests.md` — entries.

## Done when
The `game/models` and `game/models_cooked` tests pass.
