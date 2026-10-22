# 11 — The editor freezes for minutes when the camera moves among trees

## Seen
"When I move the camera I get bad perf. The editor freezes for a long time and then works smooth again.
Until I move the camera between trees or foliage." The freeze lasts a minute or two. Light bounces at 0
does not make it better, so it is not the bounce (the hitch lines agree: every bounce capture is 1–3 ms).

What the editor's hitch lines (0458) showed through one freeze, read by the tech lead, not explained:

- **CPU, shadows.** While the camera moves, the frame's `views` stretch is 150–1150 ms, and nearly all of
  it is `shadows` (77–1020 ms).
- **CPU, open.** For many frames in a row the frame takes 249.5 ms almost exactly, with `open` at 230–235 ms
  of it; once `open` took 2584 ms. The steadiness looks like a wait that runs out, not work.
- **Card, foliage.** Every card hitch is the view drawing the foliage: `view target 1: foliage` 40–107 ms,
  terrain 5–15 ms. The card's mean climbs from 17 to 31 ms over the freeze, the frame's mean from 63 to
  193 ms.

A sample, verbatim:

```
editor: hitch: frame 1289.4 ms (mean 193.1): views 1154.9, shadows 1019.8, open 96.6, step 37.7, interface 28.3
editor: hitch: frame 2624.4 ms (mean 114.3): open 2583.7, views 10.4, shadows 9.4, step 0.8, interface 0.4
editor: hitch: frame 249.5 ms (mean 157.7): open 235.2, views 16.3, shadows 15.0, end 0.7, interface 0.7
editor: hitch: card 137.3 ms (mean 23.0): view target 1 121.3, view target 1: foliage 106.5, view target 1: terrain 14.6, bounce capture 4 3.1, bounce capture 1 2.5
editor: hitch: card 53.4 ms (mean 31.6): view target 1 47.1, view target 1: foliage 41.6, view target 1: terrain 5.4, bounce capture 1 1.0, bounce capture 2 1.0
```

## Expected
Flying among the painted trees in lumberhill stays smooth: no freeze, no stretch of a quarter-second frames,
with bounces at 0 or at 1. The hitch lines name the cause of each of the three findings above, and each is
fixed or, if it is the cost of drawing the forest at all, brought under a frame. The planner may find that
what is off screen is not drawn (work order 095) is the fix for part of this; if so, it says so in a
`needs-decision.md` rather than pulling 095 in.

## How to reproduce
1. Start the editor from a terminal and open the lumberhill project's scene with `Hill.landscape` and its
   painted trees.
2. Fly the camera in among the trees and keep moving between them.
3. The editor freezes, runs at a few frames a second for a minute or two, then smooths out until the camera
   moves among other trees. `editor: hitch:` lines on stderr as above.
4. Set the Light's bounces to 0 and repeat: the same.
