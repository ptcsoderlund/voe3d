# 19 — A Scene list drag shows its ghost, its dimmed row and its drop rim
folder: editor
decisions: 0168, 0282, 0194, 0196, 0177

## Change
Needs card 18. Bug 01, the drawn half: 0282 points 2–4, from the fields card 18 added to
`voe_editor_scene` (`list_held`, `list_dragging`, `list_target`, `list_target_heading`). Only
public `ui` calls: `voe_ui_theme_push`/`_pop`, `voe_ui_panel_begin`, the anchor on
`voe_ui_container` (`ui/include/ui/widgets.h`, `ui/include/ui/layout.h`, their comments only).

- `editor/src/dock.h`, `editor/src/dock.c`: `voe_editor_panel_draw` gains
  `const voe_ui_theme *palette`, handed from `voe_editor_dock_walk`'s own; the walk's comment no
  longer says no panel is handed it. Only the Scene list reads it.
- `editor/src/scene.h`: two `voe_ui_theme` fields, `list_dim` and `list_rim`, rebuilt by the list
  each frame from the palette; kept on the scene because a pushed theme must outlive the frame.
- `editor/src/scene_list.h`, `editor/src/scene_list.c`:
  - `voe_editor_scene_list_draw` gains `const voe_ui_theme *palette`; derives `list_dim`
    (`inverse` = `control`, `control_hovered` = `control`, text roles and `inverse_ink` =
    `text_disabled`) and `list_rim` (`border` and `surface_raised` = `inverse`).
  - Every row and the "Scene" heading sit in a keyed panel (`"row_rim"`, row index; `"heading_rim"`,
    0) padded by a named rim width (about 0.5 mm) on all sides, surface NONE; RAISED under
    `list_rim` for `list_target`'s row, or the heading while `list_target_heading`. `heading`
    records the heading's wrapper. The row whose entity is `list_held` is made under `list_dim`
    while `list_dragging`.
  - New `void voe_editor_scene_list_ghost_draw(voe_ui_context *ui,
    const voe_editor_scene *scene, voe_math_float2 at);` — while `list_dragging` and the held
    entity is alive with an identity: an anchored RAISED panel, not blocking the pointer, a few
    mm right of and below `at`, holding a label of its name (the identity row's bytes).
  - Header: the three marks, the rim instead of an accent (0282), and why the wrappers are always
    there (keys and spacing unchanged).
- `editor/src/interface.c`: call the ghost draw in the root surface after the dock and before the
  overlays, with `root->pointer.at`.
- `editor/src/interface.h`: the node and element capacities grow by the wrappers (one node per
  row plus the heading, two elements for the lit rim) and the ghost (two nodes, a panel's two
  elements and a name's 64 characters); the comment's running sums say so.
- `editor/src/src.md`: the `scene_list`, `dock` and `interface` entries mention the marks, each
  under 300 characters.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0,
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" && test
-s "$d/t.png"` exits 0, and `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints
`FINDINGS: 0`. The human's: bug 01's five reproduction steps, in both built-in themes, now show
the ghost, the dimmed `tank_head`, a rim on `tank_body` and on "Scene", none over `tank_head`
itself; Escape clears them and the release does nothing; a plain click only selects.
