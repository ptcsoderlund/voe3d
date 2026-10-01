# 11 — The editor views each feed their own bounce
folder: editor
after: 10
decisions: 0168, 0307, 0308
read: feature.md

## Change
0308 points 3 and 7: each view and the preview update their own target's
grid. Read `editor/src/view_passes.h` (the capacities comment and
`VOE_EDITOR_CAPACITIES`), `editor/src/view_passes.c`, and the shadows
call's and `voe_3d_frame`'s comments in `3d/include/3d/draw_system.h`.

- `view_passes.c`: the frame handed to `voe_3d_draw_system_shadows` sets
  `.target` to the preview's target in the preview, and to the view's target
  for each view, so no view reads a grid another view scrolled. The header
  comment gains the point.
- `view_passes.h`, `VOE_EDITOR_CAPACITIES`: `.passes` gains one bounce pass
  per view and the preview, (`VOE_EDITOR_VIEWS` + 1); `.objects`' caster term
  counts one more pass per view and the preview, 2 × max drawn ×
  (cascades + 1) × (views + 1). `.targets` is unchanged; the comment at the
  top says each target also carries a probe grid.

## Done when
`grep -n '\.target' editor/src/view_passes.c` shows the preview's and each
view's frame setting it, and the folder's build passes.

The human, in the editor and the tank game (feature.md, How to test):
1. A bright red wall in full sun beside grey ground: the ground near the
   wall turns pink, fading with distance.
2. Turning the sun away from the wall: the pink fades within a second.
3. A deep shadow next to lit ground is lit softly by the ground.
4. A tank driven past the wall in Play: the bounce follows without flicker.
5. The tank game at full action stays smooth.
