# 54 — Roughness and Metal are greyed while an ORM map is set
folder: editor
after: 52, 53
decisions: 0168, 0401, 0402

## Change
0401 in the Inspector: while the open material's `ormmap` is set, its Roughness and Metal rows are
drawn greyed with card 53's `voe_ui_slider_shown` and nothing reads them, so the values in the file
stand untouched; emptied, the rows are live sliders again showing the kept values. Nothing else in
the editor changes: the surface itself already follows the map (card 52).

- `editor/src/inspector_material.h` — `voe_editor_inspector_material` gains `voe_ui_theme dim`, the
  theme the greyed rows are pushed under (here because a pushed theme must outlive the frame). New
  `void voe_editor_inspector_material_dim(voe_editor_inspector_material *controls,
  const voe_ui_theme *palette);` derives it each frame: the palette with `text_primary`,
  `text_secondary` and `inverse_ink` as `text_disabled`, `control_hovered` and `inverse` as `control`
  (as `scene_list.c`'s `marks_derive` dims a row). The header's opening says the two sliders are
  greyed and unread while ORMmap is set (0401, 0402); the read's comment says "each drawn slider",
  which a greyed row is not.
- `editor/src/inspector_material.c` — in the draw, when `material->ormmap` is not empty: push
  `controls->dim` round both rows, label, `voe_ui_slider_shown` at the slider's width and the figure,
  and leave `controls->sliders[i]` as `VOE_UI_NODE_NONE`, so the read takes nothing from them.
  Empty: as today. `voe_editor_inspector_material_dim` lives here. Fix the file's header to match.
- `editor/src/dock_walk.c` — where the Inspector panel is drawn, with the `palette` already in scope:
  `voe_editor_inspector_material_dim(&scene->material_controls, palette)` before it.
- `editor/src/src.md` — the `inspector_material` lines: greyed while ORMmap is set.

## Done when
`grep -n 'voe_ui_slider_shown' editor/src/inspector_material.c` and
`grep -n 'voe_editor_inspector_material_dim' editor/src/dock_walk.c` each find the call, and the
editor builds.

Human, the bug's steps: open lumberhill, look at the hill up close with the sun low and in front;
select `Assets/Hill/Hillmat.material`: the ground looks like dry, matte dirt untouched; Roughness and
Metal are greyed, dragging or clicking them does nothing and the ground does not change; clear
ORMmap: the sliders light up showing 0.5 and 0, and dragging Roughness changes the shine; put
`dirt_orm.png` back: matte again whatever Roughness was left at; Play: the hill looks the same; a
material with Metal 0 and an ORM map whose blue is white looks like metal; a glTF model with a
metallic-roughness texture shows its roughness and metal whatever its factors say.
