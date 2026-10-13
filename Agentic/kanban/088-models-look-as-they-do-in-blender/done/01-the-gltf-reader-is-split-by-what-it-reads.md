# 01 — The glTF reader is split by what it reads
folder: assets
after: none
decisions: 0168

## Change
`assets/src/model_gltf.c` is 1356 lines and cards 02 and 03 change it. Split it by function, moving code
only: no behaviour or message changes, and no names but the shared helpers' prefix.

- `assets/src/model_gltf_read.h` (new, internal, header comment): `struct gltf` and the helpers more
  than one file calls (`refuse`, `malformed`, `unsupported`, the `*_member` readers, `object_at`,
  component sizes, `check_buffer`, the accessor resolve and reads), made non-static with a
  `voe_assets_gltf_` prefix (rule 7), and one call per file below. Say why the split is by what is read.
- `assets/src/model_gltf.c`: keeps the shared helpers' bodies, `check_version`, `check_extensions` and
  `voe_assets_model_from_gltf`; its header comment keeps the rules that bind every part (lengths checked,
  no recursion, the one buffer) and points to the files below.
- `assets/src/model_gltf_materials.c` (new): texture references, the alpha mode, materials and images.
- `assets/src/model_gltf_meshes.c` (new): attributes, primitives and meshes.
- `assets/src/model_gltf_nodes.c` (new): the two matrix helpers and the node tree.
  Each new file opens with a header comment on what it reads and the rules from the old header that
  are its own (triangle lists and generated indices to meshes; textures resolving to images to
  materials; the tree check to nodes).
- `assets/src/src.md`: the `model_gltf.c` entry follows the move; one entry per new file, each at most
  300 characters.

## Done when
Every file under `assets/src/` is under 800 lines, and `assets/model` and the folder's other tests pass
unchanged after the folder's build.
