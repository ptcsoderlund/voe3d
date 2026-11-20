# 02 — A tree grows: skeleton, then a mesh per level
folder: 3d
after: 01
decisions: 0168, 0514, 0515, 0513, 0510

## Change
New `3d/include/3d/tree_grow.h`, with `3d/src/tree_grow.c` for the skeleton and `3d/src/tree_mesh.c` for
the meshes. Read `assets/include/assets/tree.h` and the header of `assets/include/assets/model.h` (the
in-memory model: primitives, meshes, materials, nodes, roots).

Types and constants:
- `voe_3d_tree_segment`: start and end (float3, metres, in the tree's own space, +Y up, base at the
  origin), the radius at each end, the parent's index (none for the trunk), and the order (0 trunk,
  1 branch, 2 twig).
- `voe_3d_tree_skeleton`: the segments and their count.
- `VOE_3D_TREE_LEVELS` 4.
- Caps on segments and leaf cards, so a tree at every key's maximum still fits. Growth stops at a cap;
  it is never an error. State the caps in the header.

`voe_3d_tree_grow(arena, const voe_assets_tree *values, uint32_t variation, voe_3d_tree_skeleton *out)`:
- Deterministic: an xorshift seeded from `variation` alone.
- The trunk rises `height` with base diameter `trunk`.
- Branches leave the trunk above `crown` × height, `branching` per metre of parent. Each is `reach` × its
  parent's remaining length, at `angle` degrees from the parent.
- `droop` bends a branch toward −Y along its length (negative lifts it). `crooked` jitters the direction
  of each segment.
- Twigs grow on branches by the same rules.

`voe_3d_tree_mesh(arena, const voe_3d_tree_skeleton *, const voe_assets_tree *, uint32_t variation,
voe_assets_model *out)`:
- One model with two materials, 0 bark and 1 leaf, and one root node per level named `tree_LOD<k>`, each
  with a mesh of a bark primitive and a leaf primitive. The levels follow 0514.
- Tubes have smooth normals. Their uv has u around the tube and v along it in metres.
- A child tube starts inside its parent's axis, with a collar ring widened to the parent's surface radius,
  so it looks joined.
- Leaf cards follow 0514: one quad each, `leaf_size` across, `leaf_density` per twig, rolled at random,
  normals bent off the crown's centre, uv 0–1.

Header comment points:
- the two steps, and why: the skeleton is where wind and chopping take it later, and it is not stored;
- determinism;
- coordinates and units;
- the levels per 0514.

Index entries in `3d/3d.md`, `3d/src/src.md` and `3d/tests/tests.md`.

New test `3d/tests/tree_grow.c`:
- the same values and variation give equal skeletons, and another variation differs;
- every child segment starts on its parent;
- level k+1 has fewer triangles than level k;
- `leaf_density` 0 gives no leaf triangles;
- every vertex lies within a box of `height` around the trunk;
- the nodes are named `tree_LOD0`..`tree_LOD3`.

## Meaning
- Scout: the bake reads `voe_assets_model`; own levels come from nodes whose names end `_LOD<n>`.

## Done when
`ctest --test-dir build/debug -R '^3d/tree_grow$'` exits 0.
