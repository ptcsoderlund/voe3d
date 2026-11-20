# 11 — Opening a species shows it alone in the view
folder: editor
after: 10
decisions: 0168, 0515, 0283

## Change
`editor/src/project.h`, with the new code in a new `editor/src/project_species.c`:
- `voe_editor_project_species_open(project, path, variation, why)`, after `_prefab_open`:
  - the level is set aside as text;
  - `prefab` holds the `.tree` path;
  - the world is made holding a sun (the untitled scene's light) and one model row at the origin naming
    `<path>#<variation>` (`voe_assets_tree_variation_path`);
  - unsaved is false.
- `voe_editor_project_species_variation(project, v)`: the row's model path becomes `#<v>`.
- `voe_editor_project_species_is(project)`: true while `prefab` ends `.tree`.
- Back is unchanged.

`editor/src/project.c`:
- Save writes no world while a species is open; the project file is still written.
- The disk take of the open prefab's file is not done for a species; card 15 takes `.tree`.

`editor/src/session.h` and `editor/src/session.c`: `voe_editor_session_prefab_open` with a `.tree` path
opens the species at variation 0. Every other refusal and clearing works as for a prefab.

`editor/src/assets_panel.c`: a `.tree` row opens as a `.prefab` row does.

Headers: in `project.h`, the PREFAB OPENED point says a species rides it (0515). Index entries in
`editor/src/src.md`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0.

The human: open a `Tree.tree`; a tree stands alone in the view; Back brings the level back as it was.
