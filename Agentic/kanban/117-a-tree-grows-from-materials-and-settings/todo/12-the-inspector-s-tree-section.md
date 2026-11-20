# 12 — The Inspector's tree section, regrowing as it is dragged
folder: editor
after: 11
decisions: 0168, 0515, 0386, 0494, 0498

## Change
New `editor/src/inspector_tree.h` and `editor/src/inspector_tree.c`. Read the header of
`editor/src/inspector_material.h` and mirror it. The section has:
- Bark and Leaf rows, each showing its path with a ×;
- a slider per number key — Height, Trunk, Crown, Branching, Reach, Angle (°), Droop, Crooked, Leaf size,
  Leaf density — with ranges from `assets/tree.h`'s constants;
- a Variations number box from 1 to 16;
- a Variation stepper, ◀ v / count ▶.

`editor/src/scene.h` and `editor/src/scene.c`, beside the open material:
- the shown copy `tree`, `tree_before` and `tree_controls`, plus the stepped `tree_variation`;
- these are set from `voe_editor_models_trees` when a species opens, cleared once
  `voe_editor_project_species_is` is false, and refreshed after the trees are read again;
- `tree_variation` is clamped when Variations falls.

`editor/src/inspector.c`: while a species is open, the tree section shows in place of the entity.

Reading the controls:
- a changed value goes into the shown copy, then to `voe_editor_models_tree_changed`, so the tree regrows
  while being dragged;
- the stepper calls `voe_editor_project_species_variation`.

Card 13 settles the undo steps and file writes.

Update the headers, and the index entries in `editor/src/src.md`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0.

The human: open a species, drag each slider, and see the tree change with no hitch. Step through
Variations 6, then set it to 1.
