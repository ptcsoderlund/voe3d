# 13 — The Inspector shows the level each view draws
folder: editor
after: 07, 08, 10, 12
decisions: 0168, 0425

## Change
0425 point 7: for a selected thing with a model row whose file is loaded, the Inspector shows,
per view in use, the level that view draws, of how many, made or own, asked with
`voe_3d_model_level` (`3d/include/3d/model_levels.h`), the very function the draws use. This is
the feature's last card.

- `editor/src/inspector_level.h` / `inspector_level.c` (new), shaped as `inspector_layers.h`:
  - `voe_editor_inspector_level`: one label buffer per view (`VOE_EDITOR_VIEWS`, `view.h`), kept
    because a label is drawn after the call that made it.
  - `void voe_editor_inspector_level_draw(voe_ui_context *ui, voe_editor_inspector_level *level,
    const voe_3d_models *store, const voe_ecs_world *world, voe_ecs_entity entity,
    const voe_editor_views *views)`: nothing unless the entity has a model row (`3d/
    model_component.h`) naming a loaded entry that is no picture, landscape or material; else a
    "Level" heading as Layers' is, then per view in use a row "View <n>: <k> of <count>, made" or
    "…, own", or "View <n>: 1 of 1" for a model without levels. The view as
    `voe_editor_view_pass_camera(&views->views[v], (voe_render_light){ 0 }).view`, the eye as the
    view's `eye`, the place as `voe_scene_transform_world(world, entity)`.
  - header: what it shows and why it asks the draws' own function, the label lifetime, its
    NODES and ELEMENTS at most (heading, and per view a row and a label of at most its buffer).
- `editor/src/inspector.h`: `voe_editor_inspector` gains a `voe_editor_inspector_level`;
  `voe_editor_inspector_draw` gains `const voe_editor_views *views` after `store`; its comment
  names the Level section after Layers.
- `editor/src/inspector.c`: after the Layers call (around its line 464) the Level section.
- `editor/src/dock_walk.c`: `inspector_panel` takes `views` from its caller (the walk already
  carries it) and passes it.
- `editor/src/interface.h`: a budget paragraph after card 12's for the Level section's nodes and
  elements, with the sums; the two defines raised by them.
- `editor/src/src.md`: entries for `inspector_level.h` and `.c`.

## Done when
`voe_editor` builds in the folder's check with no warning. The human runs the feature's How to
test, steps 1–6, and every step holds; in particular:
- step 2: place an imported tree, select it, fly slowly away: the Inspector's level climbs and
  the tree never pops;
- step 4: a tree exported from Blender with `_LOD1`, `_LOD2` nodes shows "own" and, flying away,
  the author's levels;
- step 6: Ship the project and run it: the far trees are drawn with few triangles there too, and
  the shipped folder holds `Cache/levels/…/<file>.levels`.
