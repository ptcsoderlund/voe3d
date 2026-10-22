# 02 — Import textures writes no picture above a largest side
folder: editor/src
after: 01
decisions: 0168, 0453, 0466

## Change
0453 and 0466 points 1–3 in the import itself. The question comes in cards 03 and 04; until then
the one caller passes 2048.

- `editor/src/model_import.h` — `voe_editor_model_import_textures` gains `uint32_t largest` after
  `glb`: the longest side any written picture may have. Header's IMPORT TEXTURES paragraph gains:
  each picture and each ORM picture written no larger than `largest` on its longest side, aspect
  kept, never enlarged, so a run at a larger size gives back up to the `.glb`'s own size (0453);
  each shrunk by its use through `voe_assets_image_fit` (`assets/image.h`), normal, colour or
  data, as 0466 point 3 says.
- `editor/src/model_import.c` — where the pictures and the ORM pictures are encoded, each first
  passed through `voe_assets_image_fit` with `largest` and its kind: NORMAL for an image any
  material's `normal_image` names, else COLOUR for one any `base_colour_image` or
  `emissive_image` names, else DATA; the ORM picture DATA. The kind is worked out once per image
  from `model.materials` (`assets/include/assets/model.h`, `VOE_ASSETS_MODEL_NONE` for an empty
  slot). The file's header says the shrink in a line.
- `editor/src/interface_assets.c` — `voe_editor_interface_assets_textures_do` passes 2048.
- `editor/src/src.md` — the `model_import.h` and `model_import.c` entries say the largest size.

## Done when
The folder check passes (the editor builds with the new argument). The human: run Import textures on
a `.glb` with 4K pictures; every written PNG is at most 2048 on its longest side.
