# 114 — A forest is cheap to draw

## What
The lumberhill forest draws in a fraction of the time it does today, and looks the same. Today, on a release
build with the scene view about 880×975 pixels, the frame breakdown shows "view target 1: foliage" at 8.88 ms
of an 11.27 ms total, for 118 trees drawn. Cutting shadows or the LODs hard changes nothing, so the cost is
not triangles or shadows. Afterwards, the same view costs at most 3 ms for foliage even with the scene view
filling the whole window. The trees, their leaves, their shadows and their LOD swaps look as they do now, in
the editor and in Play.

## Why
A small forest already takes most of the frame, and a bigger window or a bigger forest would fall below
60 fps. The engine is meant to carry forests.

## How to test
1. Release build. Open lumberhill_project and the Frame breakdown. Stand where the earlier screenshot stood:
   a stand of about 120 "tree asset.glb" trees filling the view. Note the foliage line.
2. Close the side panels so the scene view fills the window. "view target 1: foliage" reads 3 ms or less,
   and the Total is well under 16.6 ms.
3. Fly into the trees until leaves fill the view right in front of the camera. Foliage stays at or under 3 ms
   and the frame never stutters.
4. Compare with how the forest looked before: leaves are just as full, their cut-out edges just as sharp,
   shadows fall the same way, and LODs still swap calmly as I fly away.
5. Press Play and walk into the forest: it looks the same and runs as smoothly.
