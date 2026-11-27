# 39 — No species view and no tree edits
folder: editor
after: none
decisions: 0168, 0520

## Change
0520 withdraws the tree maker. Take out of `editor/` what these commits added, seen with
`git show <hash> -- editor`: card 11 `8f09faf5` (opening a species alone in the view), card 12 `1c08b5f9`
(the Inspector's tree section), card 13 `d0dadd2d` (tree undo steps), card 14 `b671167f` only its drop of a
material onto Bark or Leaf, card 24 `7167f7d6` only its `inspector_tree` sliders, and fix `1ec4325b`.
Later cards' changes to the same files stay; card 10 split the project file since, so a removal may land
in a part of it.

- Delete `editor/src/inspector_tree.c`, `inspector_tree.h`, `tree_steps.c`, `tree_steps.h`,
  `project_species.c`.
- In `scene.h`, `scene.c`, `inspector.c`, `inspector.h`, `interface.c`, `interface_read.c`,
  `interface_assets.c`, `assets_manage.c`, `dock_walk.c`, `frame_commands.c`, `frame_commands.h`,
  `undo.c`, `undo.h`, `project.c`, `project.h`, `project_prefab.h`, `session.c`, `session.h`,
  `assets_panel.c`, `assets_panel.h`, `assets_drag.c`, `assets_drag.h` (all in `editor/src/`): remove the
  open-species state, the tree section, tree undo steps, the Bark and Leaf drop targets, and every call to
  them.

Card 14's drop of a `.tree` file onto a foliage kind stays; card 40 takes it out. Header points naming a
species, the tree section or tree steps go. Their entries in `editor/src/src.md` go.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -rlE 'inspector_tree|tree_steps|project_species' editor/src` prints nothing.
