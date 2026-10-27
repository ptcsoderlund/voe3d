# 04 — R steps Move, Rotate, Scale
folder: editor
after: 03
decisions: 0168, 0482, 0274

## Change
0482 point 3 in the editor: the scene holds a `voe_3d_gizmo_kind` where it held `rings`, R steps
Move → Rotate → Scale → Move, the top bar names the mode and the views draw the scale gizmo.
This mends the build card 03 broke. Scaling by the gizmo is card 05: in Scale mode nothing is
hovered or grabbed yet.

- `editor/src/scene.h` — field `bool rings` becomes `voe_3d_gizmo_kind gizmo_kind` (zeroed is
  Move); the header paragraph on the gizmo's mode and `voe_editor_scene_gizmo_switch`'s comment
  say it cycles three, still the person's, never saved or undone.
- `editor/src/scene.c` — `voe_editor_scene_gizmo_switch` steps to the next kind, Scale back to
  arrows.
- `editor/src/gizmo.h` — `bool turning` becomes `voe_3d_gizmo_kind kind`, the mode the held drag
  began in; the header's opening says arrows, rings or (card 05) scales, by scene.h's
  `gizmo_kind`.
- `editor/src/gizmo.c` — every `scene->rings` and `turning` read becomes a kind compare: a
  change of kind mid-drag still ends the drag; with SCALES no handle is hovered or taken.
- `editor/src/view_passes.c` — `.rings = scene->rings` becomes `.kind = scene->gizmo_kind`.
- `editor/src/view_passes.h` — `VOE_EDITOR_GIZMO_VERTICES` / `_INDICES` take the largest of the
  arrows', rings' and `VOE_3D_GIZMO_SCALE_*` (include `3d/gizmo_scales.h`); comments follow.
- `editor/src/interface.c` — the top bar's mode label is "Move", "Rotate" or "Scale" by kind.
- `editor/src/topbar.h`, `editor/src/shortcuts.h` — the comments naming "Move or Rotate" and
  "arrows become rings" name the three.
- `editor/src/src.md` — the `scene`, `gizmo`, `topbar` and `view_passes` entries follow.

## Done when
The editor builds, and `grep -rnE "scene->rings|\.rings =|->turning" editor/src` prints nothing.
