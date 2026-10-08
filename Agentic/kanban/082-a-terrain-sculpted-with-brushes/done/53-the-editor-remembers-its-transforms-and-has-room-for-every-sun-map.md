# 53 — The editor remembers its transforms and has room for every sun map
folder: editor
after: 50, 52
decisions: 0168, 0389

## Change
The editor's world, made by `voe_game_world_new`, has a previous table but never fills it. So a box
dragged in the editor marks no stale probes, and the bounce keeps its old picture: bug 05's curved edge.

- `editor/src/world_step.c`: `voe_editor_world_step` calls `voe_scene_transform_remember(world)` first,
  before `voe_game_world_step`. Then this frame's edits, applied by the step, are a move between lag 1 and
  lag 0. A shape added this frame has no remembered row, so it marks as new.
- `editor/src/world_step.h`: a paragraph, "REMEMBERED FIRST", saying why: the bounce's stale spheres
  (0389), and that the editor draws at lag 0, so its picture does not change. Add it to the one-step order
  in the first paragraph.
- `editor/src/view_passes.h`, `VOE_EDITOR_CAPACITIES`: a frame that relights may open one bounce sun map
  per volume per casting sun (0389). In both `.passes` and `.objects`, the bounce shadow term
  `VOE_RENDER_DIRECTIONAL_LIGHTS` becomes `VOE_RENDER_BOUNCE_VOLUMES * VOE_RENDER_DIRECTIONAL_LIGHTS`. The
  comment above says so.
- `editor/src/src.md`: the `world_step` entries mention the remember.

Per frame (0388): on the CPU, before any pass, one copy of every transform row. A game already pays this
each step.

## Done when
`awk '/voe_scene_transform_remember\(/{r=NR} /voe_game_world_step\(/{g=NR} END{exit !(r && g && r<g)}'
editor/src/world_step.c` exits 0, and
`grep -c VOE_RENDER_BOUNCE_VOLUMES editor/src/view_passes.h` prints at least 2.
