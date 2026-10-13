# 0414 — Haze starts at a distance and leaves nearer surfaces alone
date: 2026-10-13
by: planner

## Decision
For 087 bug 06.

1. The sky row gains **`haze_start`**, metres, at least 0, after `haze`. A surface nearer the eye than
   `haze_start` takes no haze at all, byte for byte as with haze 0. Beyond it the fade is 0405 point 6's
   with the distance counted from the start: `f = 1 − 2^(−max(d − haze_start, 0) / haze)`, `d` the
   surface's distance from the eye. `haze` keeps its meaning (metres, now past the start, at which half
   a surface's colour is haze; 0 none) and its colour (0411 point 4, 0412 point 2).
2. The default and unsaid row's `haze_start` is **100 m**, so a sky added to a world, or a scene saved
   before this field, leaves everything within 100 m untouched. `haze_start` 0 is the fade as before.
3. `voe_render_sky` carries it in its first reserved word: `haze_start` at offset 52, `reserved[2]`
   after it; the record stays 96 bytes. A pass asserts it finite and at least 0. Zeroed is still the
   old picture. `3d` copies it from the row as it copies `haze`.

## Reasoning
An exponential fade counted from the eye touches every surface: with `haze` at the 50 m the sponsor set
on the hill, ground 20 m away is a quarter haze and the cube and slab wash towards the horizon colour,
which reads as the whole scene lit up. A start distance makes 0411's "far surfaces fade" true by
construction while the far end still melts into the horizon. Alternatives:
- **Haze as a density per kilometre:** removes the jump where a tiny `haze` floods everything, but every
  surface still takes some haze, and every saved value would change meaning.
- **A linear fog between a start and an end:** two new meanings for one knob already authored, and a
  hard edge at the end where the far ground meets the sky.
- **A fixed start, not authored:** a hill seen from a tower and a room-sized scene want different ones.

## Replaces
Amends 0405 point 6 and 0411 point 4: the haze's distance is counted from `haze_start`, not the eye.
