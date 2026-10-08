# 22 — The editor depends on assets
folder: editor
after: 21
decisions: 0168, 0380

## Change
`editor/CMakeLists.txt`: add `assets` to the `voe_executable(editor DEPENDS ...)`
list, after `authoring` (dependencies run high to low). This makes legal the
`<assets/landscape.h>` includes in `editor/src/assets_manage.c`,
`editor/src/game_tree_landscapes.c` and `editor/src/landscape_panel.c`; do not
open those files. If `editor/editor.md` lists the editor's dependencies, add assets
there with the reason (reads and writes landscapes, 0380).

## Done when
`grep -E 'DEPENDS.*\bassets\b' editor/CMakeLists.txt` exits 0, and
`cmake --preset debug` configures with no "not an allowed
dependency" error (exit 0).
