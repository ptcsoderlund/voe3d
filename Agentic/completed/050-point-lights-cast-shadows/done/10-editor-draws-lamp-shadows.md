# 10 — The editor's views draw the lamps' shadows
folder: editor
after: 08, 09
decisions: 0168, 0325
read: feature.md

## Change
0325 points 6 and 7. The Inspector's checkbox needs nothing: a BOOL field shows as one (card 06).
- `editor/src/view_passes.c`: in both the preview and the view draws, the arena mark, then
  `voe_3d_draw_system_point_lights`, then `voe_3d_draw_system_shadows`, then the pass begin and
  the rewind, so the points outlive the shadows call. Header phrase on the order.
- `editor/src/view_passes.h`: the capacities macro gains `.point_shadow_size =
  VOE_3D_POINT_SHADOW_TEXELS`, one more pass per `VOE_EDITOR_VIEWS + 1`, and one more object per
  drawn thing per view (`VOE_RENDER_SHADOW_CASCADES + 1` becomes `+ 2` in `objects`).
- `editor/src/src.md`: `view_passes.c`'s entry, if it names the order.

## Done when
- `awk '/draw_system_point_lights/{p=NR} /draw_system_shadows\(/{if(!p||p>NR)exit 1; p=0} END{exit 0}' editor/src/view_passes.c` exits 0.
- The human, on a hardware card: `## How to test` steps 1–6 of `feature.md` in the editor and
  in Play.
