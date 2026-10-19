# 02 — Models load slowly at startup and again at play

## Seen
Loading models is slow when I start the editor, and slow again when I press play. Iteration speed is no
good like this.

The project is lumberhill, which holds the fir tree from bug 01
(`Assets/Lanscapestuff/fir_tree_01_4k.glb`: 499 MB, about 4 million triangles, six embedded 4k PNGs).
No timings were taken.

## Expected
Opening a project and pressing play do not make the artist wait on models that have not changed since
they were last loaded. Play uses the models the editor already holds rather than loading them a second
time. Only a model whose file changed is read again, and its LOD levels are made again only if they
need to be.

## How to reproduce
1. Open the lumberhill project in the editor and note how long it takes before you can work.
2. Press play and note how long it takes before the game runs: the models load again.
