# 17 — The game tree cooks the trees table
folder: editor
after: 16
decisions: 0168, 0515, 0470

## Change
`editor/src/game_tree_materials.c` and the header of `editor/src/game_tree.h`: the cooked
`materials.c` also defines `voe_game_trees_cooked`:
- every `.tree` under `Assets/` (`game_tree_find.h`), read leniently;
- each written with its `Assets/` path, its bark and leaf paths, floats in hex as the materials' are, and
  its variations.

Update the index entry in `editor/src/src.md`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0.

The human: press Play in a project with a species. `Build/game/materials.c` holds
`voe_game_trees_cooked` with the species, and the game builds.
