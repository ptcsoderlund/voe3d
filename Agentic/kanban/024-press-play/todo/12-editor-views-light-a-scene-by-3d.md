# 12 — the editor's views are lit by 3d's answer
folder: editor
decisions: 0168, 0238

## Change
The editor's own views are lit by `voe_editor_view_light`, which returns a zeroed light — every
surface black — for a world with no light. It becomes a call to `3d`'s owner, so a lightless scene
shows unshaded in the views and in the camera's corner picture (the preview already overwrites
`frame.light` with this light, and `voe_3d_draw_system_frame` no longer asserts after card 11).

- `editor/src/view.c` — `voe_editor_view_light` returns `voe_3d_draw_system_light(world)`; its own
  count test and row copy go.
- `editor/src/view.h` — the comment on `voe_editor_view_light`: the world's light as `3d` gives it,
  unshaded when there is none (0238), not black; drop the "every surface black" and "a broken or
  half-built scene" wording.
- `editor/src/view_passes.c` — only if its comments claim a scene must have a light; the code stays.

`main.c` is not touched: it keeps calling `voe_editor_view_light`.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
2. `grep -c voe_3d_draw_system_light editor/src/view.c` prints `1`, and
   `grep -c voe_scene_light_count editor/src/view.c` prints `0`.
