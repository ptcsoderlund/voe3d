# 0290 — No light is a zeroed render light, and the editor picks the preview
date: 2026-09-29
by: planner

## Decision
For 038 bug 04, how 0287 and 0289 are carried out:

1. **`3d` answers "no light" with a zeroed `voe_render_light`**: `voe_3d_draw_system_light` and
   so `voe_3d_draw_system_frame` give intensity, colour, fill and direction nought and `unshaded`
   nought. `render` already draws that light black on lit surfaces; unlit materials and panels
   read no light and draw as before.
2. **A light of strength nought casts nothing**: `voe_3d_draw_system_shadows` opens no pass and
   leaves `shadow` zeroed when the frame's light is `unshaded` or its intensity is nought, as it
   does for a blind frame.
3. **`render` keeps `unshaded`**, set by no folder today: a pass that wants base colours may still
   ask for it. No `render` code, shader or layout changes; only the text that tied it to 0238.
4. **The editor picks the preview light**: `voe_editor_view_light` gives
   `voe_editor_project_preview_light()` when the world's light table has no rows
   (`voe_scene_light_count`), and `3d`'s answer otherwise. The preview is built in
   `editor/src/project.c` from the same values the untitled scene's `Light` is made with, its
   direction `voe_scene_light_direction` of the untitled light's rotation.

## Reasoning
The game's frame and `dev` need no change of their own: black is what `render` already draws for a
zeroed light. Keeping the decision in `editor` keeps the preview the editor's alone (0287, 0289).
Rejected: removing `unshaded` (a shader, layout and test change across `render` for no product
gain); a preview light in `3d` (the values are the editor's, 0289); testing intensity instead of
the table in the editor (a real light turned to nought would be replaced by the preview).

## Replaces
Nothing.
