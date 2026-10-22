# 12 — The bounce stalls the shadows call, and a hitch frame does not add up

## Seen
The hitch lines that card 92 split into parts, taken in lumberhill among the painted trees. These are
read off the lines, not explained, and the lines do not say whether bounces were at 0:

- **CPU, shadows: bounce.** When `shadows` is the hitch, almost all of it is the bounce part:
  `shadows: bounce` 538.7 of 543.1 ms, and 1025.0 of 1408.2 ms. The card spends only 1–6 ms per bounce
  capture, so the stall is on the CPU side of the bounce, not its drawing. `shadows: ground` reached
  230.1 ms once.
- **CPU, the frame's stretches.** `open` no longer appears; `pace` does. In one line `pace` is 8002.0 ms
  inside a 4066.1 ms frame, so a stretch is longer than its frame. In another the frame is 14462.1 ms and
  its named stretches add up to about 1000 ms, which leaves 13 s unnamed. Until the stretches add up to
  the frame, the lines cannot name what takes the time.
- **Card, foliage.** The same pattern as bug 11: `view target 1: foliage` takes 40–104 ms, terrain 8–30 ms,
  at 5.65–5.74 million triangles in 640 draws.

The lines, verbatim:

```
editor: hitch: frame 14462.1 ms (mean 134.6): pace 705.9, step 106.4, views 69.3, shadows 65.0, fence 55.2
editor: hitch: frame 938.4 ms (mean 373.4): views 544.1, shadows 543.1, shadows: bounce 538.7, shadows: ground 3.7, pace 0.8
editor: hitch: card 174.6 ms (mean 26.8): view target 1 140.6, view target 1: foliage 104.4, view target 1: terrain 29.6, bounce capture 3 5.8, bounce capture 4 4.7, 5677969 triangles, 640 draws
editor: hitch: card 85.8 ms (mean 29.3): view target 1 71.2, view target 1: foliage 58.0, view target 1: terrain 10.8, bounce capture 4 2.9, bounce capture 1 2.1, 5652165 triangles, 640 draws
editor: hitch: card 61.8 ms (mean 30.2): view target 1 51.0, view target 1: foliage 41.5, view target 1: terrain 7.7, bounce capture 4 1.7, bounce capture 1 1.6, 5649503 triangles, 640 draws
editor: hitch: frame 4066.1 ms (mean 388.1): pace 8002.0, views 1619.9, shadows 1408.2, shadows: bounce 1025.0, shadows: ground 230.1
editor: hitch: card 60.7 ms (mean 30.8): view target 1 49.4, view target 1: foliage 39.8, view target 1: terrain 7.7, bounce capture 4 2.2, bounce capture 1 1.6, 5742919 triangles, 640 draws
```

## Expected
A hitch frame's stretches account for the frame: none is longer than its frame, and no large share goes
unnamed. The CPU side of the bounce never stalls the shadows call for a visible frame. The foliage's card
time is the open part of bug 11; if the fix is that what is off screen is not drawn (work order 095), the
planner says so in a `needs-decision.md` and does not pull 095 in.

## How to reproduce
1. Start the editor from a terminal and open the lumberhill project's scene with `Hill.landscape` and its
   painted trees.
2. Fly the camera in among the trees and keep moving between them.
3. Watch the `editor: hitch:` lines on stderr for `shadows: bounce` and `pace`, as above.
