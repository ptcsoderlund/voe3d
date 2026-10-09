# 02 — Flying over a landscape stutters

## Seen
Rendering is not smooth in the editor on a project with a 1000 m landscape of 512 cells (`Hill.landscape`), on an
RTX 4070 Laptop GPU, debug build. The Frame panel showed, at one frame:

```
shadow light 0 cascade 0   0.09 ms
shadow light 0 cascade 1   0.07 ms
shadow light 0 cascade 2   0.29 ms
shadow light 0 cascade 3   1.29 ms
bounce capture 1           3.69 ms
bounce capture 2           2.39 ms
bounce capture 3           2.38 ms
bounce sun shadow          0.24 ms
bounce relight             0.11 ms
bounce capture 4           1.86 ms
bounce sun shadow          0.13 ms
bounce relight             0.09 ms
view target 1              5.55 ms
view target 1: terrain     4.94 ms
interface window           0.72 ms
Total                     19.73 ms
```

The four capture passes are 10.3 ms, more than half the frame.

A headless run of the same shape outside the editor (a 1000 m, 512-cell landscape with a 40 m hill, a sun of
bounces 1 that casts, and a still camera 20 m up) gave:

- Camera still: four capture passes every frame for about the first 450 frames, while the four volumes fill,
  then none. Only the four cascades stay. Nothing keeps marking the landscape stale.
- Camera moving 0.15 m a frame (about 9 m/s at 60 fps) after it settled: frames go irregularly between four
  capture passes with one or two relights and no capture pass at all, because the nests about the eye move and
  queue a slice of probes each time (`3d/include/3d/bounce_grid.h`, "a flying one a slice at a time"). The
  breakdown above, three captures, a relight, the fourth capture and a second relight, is one of those frames.

So a moving camera gives frames of about 20 ms next to frames of about 10 ms, and that unevenness is the stutter.

## Expected
Flying or moving the camera over a landscape gives even frame times. The probes may fill in over more frames,
but no single frame pays for four capture passes while the camera moves.

The terrain's own 4.94 ms in the view is high for this card too, and is worth a look while here: how many nodes
the view draws at 512 cells, and whether the captures draw the landscape at the view's detail.

## How to reproduce
1. Open a project with a 1000 m landscape of 512 cells and a sun with bounces 1 that casts shadows.
2. Open the Frame panel.
3. Wait about ten seconds with the camera still; the bounce capture passes disappear from the breakdown.
4. Fly the editor view across the landscape.
5. The bounce capture passes come back on some frames and not on others, and the view does not move smoothly.
