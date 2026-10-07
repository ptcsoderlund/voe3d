# 083 — The terrain reaches the horizon

## What
A terrain can be large, several kilometres a side, and is seen all the way to its edge. Near ground is
detailed, and far ground is drawn with less detail. There are no cracks or holes between near and far,
and the change in detail is not seen as a jump while flying. The frame breakdown shows what the terrain
costs. Sculpting still works on a large terrain.

## Why
The view from the lookout tower is the game's finish (0374). It needs a valley out to the horizon.

## How to test
1. Make a terrain 4 km a side. Sculpt a hill in the middle and some ridges near the edges.
2. Fly to the top of the hill and look around. The ridges at the edges show, with no gaps or holes.
3. Fly slowly from the edge to the hill, low over the ground. I do not see the ground jump or crawl as
   detail changes.
4. Fly fast over the whole terrain on my laptop. It stays smooth. The frame breakdown names the terrain's
   share of the frame.
5. Sculpt near the edge and in the middle. Both work as in 082.
6. Press Play, and ship the project: the same view in both.
