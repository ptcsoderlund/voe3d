# 04 — The bake takes one level of a file's own levels
folder: 3d
after: 01, 03
decisions: 0168, 0425

## Change
0425 point 4: a Blender file's own levels are told by node names, which `assets` now reads (card 01:
`voe_assets_node.name`). The bake learns to bake one level and to say how many the file has.

- `3d/src/model_bake.h`:
  - `voe_3d_model_bake_create` gains `uint32_t level` after `model`: only primitives of nodes of that
    level are baked; parts are still one per material, in the order met within that level.
  - new `uint32_t voe_3d_model_bake_levels(const voe_assets_model *model)`: one more than the
    highest own level any node carries, so 1 for a file with none.
  - header: a paragraph on a node's level as 0425 point 4 gives it: a name ending `_LOD<n>` (LOD in
    any case, n all digits) makes the node and its subtree level n, no suffix inherits the parent's,
    roots level 0, n above 3 left out with its subtree; the store matching a level's parts to level
    0's by material is the store's, not the bake's. Usage example shows the level argument.
- `3d/src/model_bake.c`: the explicit-stack walk carries each node's level beside it; a node's
  suffix read from its name; a node whose level is not the one asked is walked (its children may
  differ) but its mesh not baked; a node above level 3 is not walked further. `_levels` walks the
  same way, counting. One small static helper reads the suffix.
- `3d/src/models.c`: its one call passes level 0. Nothing else changes here.
- `3d/src/src.md`: the `model_bake.h` entry names the level and the level count.
- `3d/tests/model_bake.c`: its existing call passes 0; a new case builds, as the file's first case
  does (designated initializers, one shared triangle), a model whose root "Tree" wears material 0,
  with children "Tree_LOD1" (material 0), "Leaves_lod2" (material 1) under which an unnamed child
  (material 0), and "Twig_LOD7" (material 0); checks:
  1. `_levels` is 3;
  2. level 0 bakes one part of 3 vertices (the root's only);
  3. level 1 bakes one part, material 0;
  4. level 2 bakes two parts, materials 1 then 0 (the unnamed child inherited level 2);
  5. a model whose nodes carry no suffix has `_levels` 1 and level 0 bakes every node, as before.
  Its header names the new case.
- `3d/tests/tests.md`: the `model_bake.c` entry names the levels.

## Done when
The test `3d/model_bake` passes after the folder's build, with the new case in it.
