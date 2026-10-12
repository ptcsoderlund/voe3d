# 19 — The editor's views take the picture
folder: editor
after: 17, 18
decisions: 0168, 0406
read: feature.md

## Change
The editor builds its views' frames by hand (commit 584b1af0 gave them the sky); they now carry the
picture too (0406 point 4). Read `editor/src/view_passes.h`, `editor/src/view_passes.c` and the
`voe_3d_draw_system_picture` comment in `3d/include/3d/draw_system.h` (card 17).

- `editor/src/view_passes.c`: the view's first frame gains `.exposure =
  voe_3d_draw_system_picture(world)` beside its `.sky`, and the drawn frame `.exposure =
  frame.exposure` beside `.sky = frame.sky`. Keep the sky lines.
- `editor/src/view_passes.h`: the header names the world's picture beside its sky.
- `editor/editor.md`: the view_passes entry, if it names the sky, names the picture. At most 300
  characters.

## Done when
`grep -c voe_3d_draw_system_picture editor/src/view_passes.c` prints 1 and `voe_editor` builds in the
folder's build.

The human's, in the editor (feature.md's How to test):
1. Open lumberhill, select an entity, Add component "Rendering / Sky": a blue sky with a paler horizon
   and a sun behind the hill, in the views and the camera preview; nothing but the sun brighter than
   white. The Inspector shows no light, exposure or tone on the sky.
2. Change zenith, horizon, ground, horizon height, sun size and sun glow: each changes as named.
3. Rotate the Light low over the horizon, sunset 0.5: the horizon on the sun's side warms and the sky
   darkens; sunset 0 keeps the authored colours.
4. Turn it back up: shaded slopes take the sky's soft blue; the Light's fill intensity raises and
   lowers it.
5. Fly to the top: far ridges in haze of the horizon's colour, nearer ground less.
6. A sphere of low-roughness metal reflects the sky.
7. Add "Rendering / Picture": Exposure brightens and darkens; with Tone Soft bright light rolls off.
8. Open `examples/tank_game`: exactly as before.
9. The frame breakdown: `view …: sky` is a small share.
10. Save, close, reopen; Play; Ship: the same picture in all three.
