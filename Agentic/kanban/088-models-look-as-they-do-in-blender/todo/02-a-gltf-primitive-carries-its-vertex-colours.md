# 02 — A glTF primitive carries its vertex colours
folder: assets
after: 01
decisions: 0168, 0421

## Change
glTF's `COLOR_0` read into the primitive (0421 point 1); nothing downstream reads it yet (card 11).

- `assets/include/assets/model.h`: `voe_assets_primitive` gains `const voe_math_float4 *colours`, one per
  vertex, linear, NULL when the primitive has no `COLOR_0`. Header: that it multiplies the base colour
  as glTF says, why float4 whatever the file stored, and that a three-channel one has alpha 1.
- `assets/src/model_gltf_meshes.c`: read `COLOR_0` beside the other attributes: VEC3 or VEC4 of FLOAT,
  or of UNSIGNED_BYTE or UNSIGNED_SHORT with `normalized` true, turned into float 0–1. Any other
  component type or shape is UNSUPPORTED with a message naming it, as other attributes are. An unindexed
  primitive's expansion carries the colours with the rest. Every length checked as the file's header
  says.
- `assets/tests/model.c` (and `assets/tests/model_data.inc` if the file is built there): a hand-built
  `.glb` whose primitive has `COLOR_0` as normalized UNSIGNED_BYTE VEC4 reads back the expected floats;
  one with FLOAT VEC3 reads alpha 1; one without reads NULL; a BYTE (signed) `COLOR_0` is UNSUPPORTED.
- `assets/tests/tests.md`: the `model.c` entry names the colour cases (at most 300 characters).

## Done when
The test `assets/model` passes after the folder's build.
