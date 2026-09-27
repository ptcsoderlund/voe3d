# 12 — The views show the sun marker
folder: editor
decisions: 0168, 0274, 0223

## Change
Needs cards 06 and 11. Picking the marker needs nothing here: `voe_3d_pick` meets it (card
04), so `editor/src/pick.c` selects the sun already.

- `editor/src/view_passes.c`: in each shown view's pass (not the preview's, which draws no
  marks, 0223), set `frame.sun` to the world's first light entity with a transform — entity,
  the outline material, the colour the camera marker is given (the outline's colour when it is
  selected, the gizmo's rest colour otherwise), the camera marker's pixels and the view's
  size. Reuse the camera marker's choice of colour rather than repeating it.
- `editor/src/view_passes.h`: the capacity macros count `VOE_3D_SUN_MARKER_VERTICES` and
  `_INDICES`, one more transient range and one more object per view pass; the gizmo's share
  becomes the larger of `VOE_3D_GIZMO_VERTICES` and `VOE_3D_GIZMO_RING_VERTICES` (indices
  likewise), for card 13. The header's list of what a pass draws names the sun marker.
- `editor/src/src.md`: the `view_passes` lines name the sun marker.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and
`build/debug/editor/voe_editor examples/coin_game --capture <scratch>/sun.png` exits 0 with a
PNG written (the marker's look is the walk-through's, card 15).
