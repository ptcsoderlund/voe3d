# 01 — A glTF node keeps its name
folder: assets
after: none
decisions: 0168, 0425

## Change
0425 point 4 reads Blender's own levels from node names (`Trunk_LOD1`); `assets` drops a node's name
today. A material's and an image's are read by a static `read_name` in `model_gltf_materials.c`.

- `assets/include/assets/model.h`: `voe_assets_node` gains `voe_assets_model_name name`, "" when the
  file gives none, cut as other names are (0247). One line on it in the node's comment.
- `assets/src/model_gltf_read.h`: declare the name read as a shared helper,
  `bool voe_assets_gltf_read_name(struct gltf *gltf, uint32_t object, voe_assets_model_name *out)`.
- `assets/src/model_gltf_materials.c`: `read_name` becomes that helper, unchanged in what it does;
  its callers here call it by the new name.
- `assets/src/model_gltf_nodes.c`: each node's `name` read with it; a name that is not a string is
  MALFORMED as a material's is. The header names the name among what is read.
- `assets/src/src.md`: the `model_gltf_nodes.c` entry names the node's name.
- `assets/tests/model.c`: a case whose file gives the child node `"name":"Trunk_LOD1"` and the root
  none reads "Trunk_LOD1" and ""; a node whose name is a number is refused. Build the JSON the way
  the file's other cases do. Its header's list of cases names it.
- `assets/tests/tests.md`: the `model.c` entry, if it lists cases, names node names.

## Done when
The test `assets/model` passes after the folder's build, with the new case in it.
