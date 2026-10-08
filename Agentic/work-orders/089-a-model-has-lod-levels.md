# 089 — A model has LOD levels

## What
A model is drawn with less detail the smaller it is on screen. Its simpler levels are made
automatically on import and kept in the project's cache, so they are made once. A model that brings its
own levels from Blender uses those instead. Changing level is not seen as a pop while flying. The
Inspector shows which level a selected model draws, and the frame breakdown shows how many triangles the
frame draws.

## Why
Trees seen from the tower number in the thousands. Far away they must cost a fraction of a near tree.

## How to test
1. Import a downloaded tree with no levels of its own. The splash or the Assets panel shows it being
   prepared once, and then it is ready.
2. Place it and fly slowly away from it. The Inspector's level goes up as it shrinks, and I do not see it
   pop.
3. Close and reopen the project. The tree is not prepared again.
4. Export a tree from Blender with my own levels and import it. Flying away shows my levels, not
   generated ones.
5. Place twenty trees and look at the frame breakdown's triangle count near and far. Far is a small
   fraction of near.
6. Ship the project and run it. The levels are used there too.
