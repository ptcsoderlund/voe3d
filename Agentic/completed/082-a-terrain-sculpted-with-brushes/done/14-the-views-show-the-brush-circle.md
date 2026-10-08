# 14 — The views show the brush circle
folder: editor
after: 05, 13
decisions: 0168, 0379

## Change
Each view draws the brush's two rings where the pointer meets the ground, and hides the gizmo while a
brush is chosen on a landscape (0379 point 3).

- `editor/src/view_passes.h` / `view_passes.c` — when the sculpt state (scene.h's `sculpt`) has a hover
  hit: each view's frame sets `brush` (3d/draw_system.h, card 05) on the hovered entity at its x and z,
  `radius`, `inner` = radius·(1 − softness), the outline's material, colour lerped from the gizmo's rest
  colour to the outline's (view.c's colours) by (strength − 0.05) / 0.95, and the markers' pixels and
  size. While a brush is chosen and the selection wears a landscape, the frame's `gizmo` is zeroed.
  `VOE_EDITOR_CAPACITIES` adds `VOE_3D_BRUSH_MARKER_VERTICES` / `_INDICES`, one transient range and one
  object per view; the header's list of what a view draws names the brush.
- `editor/src/src.md` — the view_passes entries name the brush circle.

## Done when
`grep -c VOE_3D_BRUSH_MARKER_VERTICES editor/src/view_passes.h` prints 1 or more, and the folder's check
passes.
Human: How to test steps 2 (the circle follows the ground) and 3 in feature.md.
