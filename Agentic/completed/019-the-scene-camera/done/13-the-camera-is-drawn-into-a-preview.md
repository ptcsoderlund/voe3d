# 13 — What the scene's camera sees is drawn into a preview target
folder: editor
decisions: 0168, 0222, 0223

## Change
Decision 0223: one 480×270 target holds what the world's camera sees, drawn only while the selected entity
has a camera. Card 14 shows it on the views; this card makes and fills it.

- `editor/src/view.h` / `view.c` — `voe_editor_views` gains `preview_target` and `preview_texture` (made in
  `voe_editor_views_create` at 480×270, the size named once as constants in view.h, refused the way a view's
  target is), and `bool preview_shown` (this frame's answer, set by the pass below). Header: what the
  preview is, its fixed size and why fixed (0223), and that nothing but the pass below draws into it.
- `editor/src/view_passes.h` / `view_passes.c` —
  - a function run each frame before the view passes, `voe_editor_view_passes_preview(...)`, that sets
    `preview_shown` to whether the selected entity has a camera (`voe_scene_camera_get`) and, when it has,
    opens one pass onto the preview target and runs `voe_3d_draw_system_run` with the frame
    `voe_3d_draw_system_frame(world, 480×270)` hands back, lit by `voe_editor_view_light`, with no outline,
    no gizmo and no marker (their records left zeroed). A frame whose `blind` is set (card 02) draws no pass
    and leaves `preview_shown` false. It returns false only when the pass is refused, as the view passes do,
    and `editor/src/main.c` calls it beside `voe_editor_view_passes_draw`.
  - `VOE_EDITOR_CAPACITIES`: one more pass and one more target; objects grow by
    `VOE_EDITOR_PROJECT_MAX_DRAWN` for the preview's world; the capacity comment says why.
- `editor/src/src.md` — the `view.h` and `view_passes.c` entries name the preview.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and
`build/debug/editor/voe_editor --capture "$(mktemp -d)/frame.png" --size 640x360` exits 0 with nothing on
stderr about a capacity.
