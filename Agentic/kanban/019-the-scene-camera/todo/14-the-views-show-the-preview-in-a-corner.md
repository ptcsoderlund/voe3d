# 14 — Every scene view shows the preview in its corner
folder: editor
decisions: 0168, 0223

## Change
Card 13 made `views->preview_texture` and `views->preview_shown` (`editor/src/view.h`). Decision 0223: while
it is shown, every drawn scene view has the picture in its bottom-right corner at 30% of the view's width.

- `editor/src/dock.c` — in the scene-view case of `voe_editor_panel_draw`, after the view's own picture:
  when `views->preview_shown`, one more `voe_ui_image` of `preview_texture`, as an anchored child
  (`ui/include/ui/layout.h`'s `voe_ui_anchor`, END on both axes, a small inset) so it paints over the view's
  picture. Its width is 30% of the view's recorded rectangle's width (`views->views[view].rect`, last
  frame's, as the view's own image is sized from), its height that width times 270/480; a view with no
  rectangle yet draws no preview. The 30% is a named constant beside its use. The node is not recorded: a
  click on it reaches the view as before. dock.c's header paragraph on the scene-view panel names the
  corner picture.
- `editor/src/src.md` — the `dock.c` entry names the corner picture.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and
`build/debug/editor/voe_editor --capture "$(mktemp -d)/frame.png" --size 640x360` exits 0.
