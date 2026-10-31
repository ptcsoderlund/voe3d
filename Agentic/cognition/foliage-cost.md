# Foliage cost

## What is wanted
- The sponsor finds the forest's foliage numbers "kinda bad": view target 1: foliage at 13.31 ms of 16.38 ms, 3,136,035 triangles, 192 draws, in a small editor scene view with "tree asset.glb" (a debug run); they asked whether we do GPU instancing. [intent · tech-lead · 2026-10-30]
- On release the forest still takes "view target 1: foliage" 8.88 ms of 11.27 ms, in a scene view of about 880×975 pixels, 3,243,207 triangles, 167 draws; the sponsor's monitor runs at 164.84 Hz. Work order 114 asks for the forest to be cheap to draw. [intent · tech-lead · 2026-10-30]
- 114's target, at most 3 ms for foliage with the scene view filling the window, is the tech-lead's number; the sponsor has not stated one. [assumed · tech-lead · 2026-10-30]

## What it does not move with
- "Even with or without shadows, heavy LOD scaling or not. fps is the same." The forest's cost does not move with shadows or with how hard the LODs cut triangles. [confirmed · tech-lead · 2026-10-30]
