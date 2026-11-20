# 08 — The editor grows the project's species
folder: editor
after: 07, 06
decisions: 0168, 0515, 0386, 0485, 0504, 0370

## Change
New `editor/src/trees.h` and `editor/src/trees.c`. Read the header of `editor/src/materials.h` and mirror
it.
- The project's `.tree` files under `Assets/` become `game/trees.h`'s table. Each is read leniently; one
  that will not read is left out, with a line on stderr.
- `voe_editor_trees_find(trees, path)` finds a row by path, for an edit in place.

`editor/src/models.h` and `editor/src/models.c`:
- The store holds a trees table and one `voe_game_trees_job`. The table is read on a different folder and
  by `_materials_read`, beside materials and parts.
- In `_update`, after the models' update and watch and before the materials:
  - with a progress, `voe_game_trees_grow` grows them all;
  - otherwise the job runs. A row marked changed is started when the job is idle and polled; when done it
    puts one variation per update; it starts again if the row changed meanwhile.
  - A row read anew or changed is marked, so a project's first update grows all of them.
- New `voe_editor_models_tree_changed(models, path, const voe_assets_tree *values)`: the row takes the
  values and is marked to regrow. It is used for a live drag, an undo and a disk take.
- New `voe_editor_models_trees(models)`: the table, read-only.
- `_destroy` waits out a running job before freeing.

Header comment: a TREES point after the MATERIALS one. Index entries in `editor/src/src.md`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0.

The human, with a `.tree` file written by hand under a project's `Assets/` and a landscape kind naming it:
open the project, paint the kind, and see trees stand.
