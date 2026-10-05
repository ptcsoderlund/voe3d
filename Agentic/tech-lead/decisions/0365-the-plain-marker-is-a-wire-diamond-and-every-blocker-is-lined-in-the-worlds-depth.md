# 0365 — The plain marker is a wire diamond, and every blocker is lined in the world's depth
date: 2026-10-05
by: planner

## Decision
For 060, how 0354 is built.

1. **The plain marker is `3d`'s `place_marker`**: a wire octahedron, its six tips 0.25 m from the
   entity's world position along the world axes, twelve edges as line quads of a fixed pixel width
   (the point light's way: world axes, position only, rotation and scale ignored). It picks by a
   world-axis cube of half extent 0.25 m about the position, as the lamp's does.
2. **Who wears it is one answer in `3d`**, `voe_3d_place_marker_wanted`: a live entity with a
   transform and no shape, model, water, mesh, panel, camera, light or point light row. The draw and
   the pick both ask it, so what is drawn is what is clicked.
3. **Water is picked on its plane** (width along the transform's X, length along Z, centred, both
   faces), so water is clicked on its mesh as 0354 says and needs no marker.
4. **A marker beats a mesh**: `voe_3d_pick` keeps the nearest mesh hit (shape, model, water) and the
   nearest marker hit (camera box, sun cube, lamp cube, place cube); any marker hit is the answer,
   else the mesh hit. The distance handed back is the answer's.
5. **Every light blocker's box is lined**: the ones not selected in the world's depth (a wall in
   front hides them) in the caller's faint colour, the selected one as today behind the outline's
   clear in the outline's colour. The editor's faint colour is the outline colour times 0.3,
   below a gizmo handle's rest (0.55).
6. **One record type for "every row, one selected"**, `voe_3d_rows_marked` (shown, selected,
   material, colour, selected colour, pixels, size), used by the frame's new `places` and
   `light_blockers` fields; `light_blockers` replaces the frame's `light_blocker`.

## Reasoning
A diamond reads as "a point here" from every side and differs from the lamp's ball, the sun's arrow,
the camera's box and a blocker's box. A cube hit, not lines, is how every marker already picks. One
predicate keeps the drawn and the clicked set from drifting. Faint lines in the world's depth keep a
level full of rooms readable; only the selected one shows through. The editor's world holds only
authored entities, so its pool sizes places by VOE_GAME_WORLD_AUTHORED.

## Replaces
nothing. Extends 0354, 0347 point 5.
