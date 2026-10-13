# 03 — A glTF material carries its name, sides, unlit, emission strength and repeat
folder: assets
after: 02
decisions: 0168, 0420

## Change
What 0420 points 3 and 4 need from the file, read and carried; nothing downstream reads it until cards
06 and 13.

- `assets/include/assets/model.h`:
  - `VOE_ASSETS_MODEL_NAME` (64) and `typedef struct { char text[VOE_ASSETS_MODEL_NAME]; }
    voe_assets_model_name`: a glTF `name`, NUL-terminated, cut at a whole UTF-8 character when longer
    (0247), "" when the file gives none.
  - `voe_assets_material` gains `name`, `bool double_sided`, `bool unlit` (KHR_materials_unlit present),
    `float emissive_strength` (KHR_materials_emissive_strength, 1 without it) and
    `voe_math_float2 colour_scale` (KHR_texture_transform `scale` on the base colour texture, 1 1
    without it; offset, rotation and `texCoord` there are not read).
  - `voe_assets_model` gains `const voe_assets_model_name *image_names`, `image_count` long, each the
    glTF image's `name`.
  - Header: the "doubleSided is not carried" paragraph replaced by what is carried now and why; the three
    extensions accepted even when required, and the unsupported list unchanged otherwise.
- `assets/src/model_gltf_materials.c`: read the five material fields, the extension objects under the
  material and under `baseColorTexture`'s `extensions`, and the image names. Every count and length
  checked as the file's header says.
- `assets/src/model_gltf.c`: `check_extensions` lets KHR_materials_unlit,
  KHR_materials_emissive_strength and KHR_texture_transform through when required.
- `assets/tests/model.c`: a hand-built `.glb` with two named materials, one `doubleSided` and unlit, one
  with emissive strength 4 and a texture transform scale 3 3 on its colour texture, and a named image,
  reads each field back; a file with none of them reads the defaults; a 100-byte name is cut to 63 bytes
  at a character boundary; a file requiring KHR_texture_transform loads.
- `assets/tests/tests.md`: the `model.c` entry grows by the material cases (at most 300 characters).

## Done when
The test `assets/model` passes after the folder's build.
