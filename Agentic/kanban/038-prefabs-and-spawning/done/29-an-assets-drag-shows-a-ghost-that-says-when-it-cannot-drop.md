# 29 — An Assets drag shows a ghost that says when it cannot drop
folder: editor
after: 28
decisions: 0168, 0277, 0283, 0285, 0286

## Change
Bug 02: dragging a model or prefab from the Assets panel shows no ghost. Give it the ghost of
card 28, named by the file and refused where its release does nothing.

- `editor/src/assets_drag.h`, `assets_drag.c`:
  - `voe_editor_assets_drag` gains `voe_math_float2 from` (the pointer at the start),
    `bool dragging` (moved past `VOE_EDITOR_SCENE_DRAG_START` from `from`, `scene_list.h`) and
    `bool refused` (the release's answer at the pointer this frame).
  - One static function giving the outcome at a pointer (one of the header's four, "nothing"
    included), used both by the release and, every frame while `dragging`, to set `refused`
    (true for "nothing": over the Assets panel, the Scene list, a model over the Inspector with
    no selected model or a part, a prefab over the Inspector, a prefab over a view while a
    prefab is open, or `blocked`). The release does nothing unless `dragging`; otherwise it is
    unchanged.
  - New `void voe_editor_assets_drag_ghost_draw(voe_ui_context *ui, const voe_ui_theme *dim, const voe_editor_assets_drag *drag, voe_math_float2 at);`
    while holding and dragging, `voe_editor_drag_ghost_draw` (`drag_ghost.h`) with the path's
    last segment as the name (0286 point 3) and `refused`.
  - Header: the start threshold, the ghost and what refused means join FOUR OUTCOMES; the
    struct's new fields documented.
- `editor/src/interface.h`, `interface.c`: `voe_editor_interface_draw` gains
  `const voe_editor_assets_drag *drag` after `scene`; beside the Scene list ghost call in the
  root, `voe_editor_assets_drag_ghost_draw(ui, &scene->list_dim, drag, root->pointer.at)`.
  Header: which ghost is drawn; the budget paragraph notes one ghost at a time and, if a file
  name can be longer than the 64 characters counted, raises `VOE_EDITOR_INTERFACE_ELEMENTS` to
  the longest name `VOE_EDITOR_ASSETS_DRAG_PATH` allows.
- `editor/src/main.c`: pass `&drag` to `voe_editor_interface_draw`; nothing else changes.
- `editor/src/src.md`: `assets_drag.h`/`.c` and `interface.c` entries amended as short phrases.

Read only these files and `drag_ghost.h`, `scene_list.h` (for the threshold), `dock.h` and
`view.h` headers for the over-a-panel and over-a-view calls `assets_drag.c` already uses.

## Done when
- `grep -c "voe_editor_assets_drag_ghost_draw" editor/src/interface.c` prints 1.
- Human (makes bug 02's steps and feature.md's How to test 1-2 true again): open
  `examples/tank_game`; drag a prefab from the Assets panel: past about 1 mm a ghost with its
  file name follows; over the Scene list, the Inspector or the Assets panel it is dimmed and
  says "Can't drop here", and releasing there places nothing; over a view it looks normal and
  releasing places a copy. A click on a prefab row still opens it with no ghost. A model
  dragged over the Inspector with a modelled thing selected looks normal and swaps its model.
