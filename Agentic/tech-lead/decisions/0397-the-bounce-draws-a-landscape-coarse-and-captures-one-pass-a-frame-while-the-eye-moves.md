# 0397 — The bounce draws a landscape coarse, and captures one pass a frame while the eye moves
date: 2026-10-09
by: planner

## Decision
For 084 bug 02, amending 0389 point 3 and 0396 point 4.

1. **Coarse in the bounce.** A landscape drawn into a bounce capture pass or a bounce sun map is selected
   from the same eye as the view's, at most `VOE_3D_TERRAIN_BOUNCE_NODES` 16 nodes (draw_terrain.h, internal to 3d);
   past that, as 0396 point 4 says, refining stops and coarser nodes are drawn. The view, the cascades
   and the point-shadow pass still draw the view's nodes, so the view's ground and its shadows agree.
   A crack between nodes two levels apart may show in a capture face or a bounce sun map; at 8 texels a
   face it is below a texel, and the probes' pictures and their sun map draw the same coarse ground.
2. **One capture pass a frame while the eye moves.** Each target's caller-kept memory
   (`voe_3d_bounce_casters`) also keeps the eye of its last bounce. When this frame's eye is more than
   1 mm from it on an axis, the frame's capture passes are at most `VOE_3D_BOUNCE_MOVING_PASSES` 1, taken
   coarse to fine by the first volume with probes queued. Otherwise, and with no memory or no eye
   remembered yet, 0389 point 3's share of `VOE_RENDER_BOUNCE_CAPTURE_PASSES` stands. render's cap of 4
   is unchanged.
3. **The view's own terrain cost** (4.94 ms in the report) is left to 093: without a frustum test the
   view draws every chosen node, those behind the camera too (0395).

## Reasoning
- Flying at 9 m/s, the 1 m nest steps a cell about every seven frames and queues a slice of 288 probes,
  18 passes, drained four a frame: four or five frames of four captures, then two or three of none.
  That alternation, 20 ms against 10 ms, is the stutter. With one pass a frame, flying demand (about
  3.5 passes a frame at that speed) exceeds supply, so every moving frame pays the same one pass. The
  probes fill in later, which the bug allows; a still eye fills four a frame again (0386: even beats
  sooner).
- A capture draws 96 faces of 8 texels; at the view's detail each pass draws every chosen node into
  each face within the volume's reach, and the level grid's reach covers a 1000 m landscape whole. That
  is why a capture costs 2.4–3.7 ms. Sixteen nodes cut the vertices of a capture by an order.
- Coarse first while moving: the coarse volumes' slices are rarer and cover more; the 1 m nest, from
  more than 7 m up, covers air (0389 point 2).
- Rejected: a fixed lower budget at all times (a still scene would fill four times slower); pacing from
  render's queue length (render does not give it, and render/vulkan is not ours to change); the eye
  kept on `voe_3d_frame` (built afresh each frame, it cannot remember).

## Replaces
Nothing. Amends 0389 point 3 (the budget while the eye moves) and 0396 point 4 (bounce passes draw a
landscape coarser than the view).
