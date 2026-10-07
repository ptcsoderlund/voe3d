# 089 — A forest is painted onto the terrain

## What
A foliage brush paints models onto the terrain: trees, rocks, bushes. I pick one or more models, a
density, and how much each copy may vary in size and turn. Painting scatters copies standing on the
ground, upright or leaning with the slope as I choose. Erasing removes them under the brush. Thousands of
copies stay smooth to fly over, because copies of a model are drawn together (0375). Sculpting the ground
under them keeps them standing on it. One stroke is one undo step. The forest is saved with the scene.

## Why
A forest placed one tree at a time is not a forest. 0376 makes foliage painted, on the editor's terrain.

## How to test
1. Pick two tree models and a rock in the foliage brush. Paint the hill's lower slopes. Trees and rocks
   appear, each a little different in size and turn.
2. Paint the whole valley with trees, many thousands. Fly over it on my laptop: it stays smooth. The frame
   breakdown shows far fewer draws than trees.
3. Erase a clearing for the house. The trees go under the brush.
4. Raise the ground under some trees. They stay standing on it.
5. Undo and redo the clearing.
6. Look from the hilltop to the far valley. Far trees are there, drawn with their simple levels (088).
7. Save, close and reopen. Press Play, and ship: the same forest in all three.
