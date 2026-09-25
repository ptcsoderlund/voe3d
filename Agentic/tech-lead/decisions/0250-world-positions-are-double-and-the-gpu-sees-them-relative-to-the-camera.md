# 0250 — World positions are double, and the GPU sees them relative to the camera
date: 2026-09-25
by: tech-lead

## Decision
The engine is designed for open worlds: nothing may assume a level fits in a few kilometres
around the origin. **A transform's position is three doubles**; its rotation and scale stay
float. Everything that places a thing in the world (the scene text, the cook, intents, picking,
the gizmo, the editor camera, collision queries) carries positions as double. **The GPU never
sees a world position**: before drawing, the CPU subtracts the camera's position in double and
hands the GPU small floats around the camera. Physics (0249) does the same: body and collider
positions are double, and the math inside a query runs in float relative to a nearby point. The
same rule holds for any later GPU reader of world data, such as particles. Collision (027) is
built this way from its first card.

## Reasoning
A float keeps about 7 digits: 1 mm steps at 10 km, 8 mm at 100 km, which shows as jitter near
the camera and physics that will not settle. Doubles for position only cost 12 bytes per
transform, and camera-relative floats keep the GPU and query math as fast as before, which is
how Unreal 5's Large World Coordinates and Jolt's double-precision mode work. Deciding before
collision exists keeps the change to the transform and the draw. Rejected: a floating origin
that shifts the world back when the player strays (every system must survive the shift, breaks
with several far-apart viewpoints); float and a capped level size (rules out open worlds);
doubles everywhere including rotation, scale and the GPU (costs memory and GPU speed for no
precision gain).

## Replaces
nothing. World streaming, which an open world also needs, is not decided here (see
`ideas.md`), and ADR-0118's rule that physics shapes load synchronously will need revisiting
with it.
