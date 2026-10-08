# 083 — A small coloured box tints the ground beside it

## What
Light bounces off small things as well as big ones. A 1 m box in a strong colour, standing in sunlight,
colours the ground on its lit side, strongest at the box and fading with distance, on a landscape and on
any other floor. Things beside each other colour each other: a red slab in front of a box shows red on
the box's face. Flat ground in bounce light is even, with no blotch, ring or curved edge that nothing in
the scene casts. Moving, adding, removing or recolouring a box, or changing the sun, settles to the same
picture as if the scene had been opened that way. None of it hitches the editor while I fly or move
things (0386, 0388).

This is 082's bug 05, cut out of 082 by 0391 after a fourth block. How it is built stands as decided:
nested bounce grids about the camera (0387, 0389, 0390).

## Why
The hill's world needs the things on it to colour the ground around them (0387). The hill was accepted
without it only so the tint could get a work order of its own.

## How to test
1. Make a scene with a flat floor (a stretched box is enough), no landscape, and a sun low in the sky at
   bounces 1, bounce strength 2.
2. Put a 1 m purple cube on the floor. The floor beside the cube's sunlit face turns pink, strongest at
   the cube and fading within a metre or two. The floor on the cube's shaded side does not.
3. Raise bounce strength to 5. The pink gets stronger. Put a thin red slab on the floor in front of the
   lit face: the cube's face takes a red tint near the slab.
4. Look at the whole floor away from the cube. It is even, with no blotch, streak, ring or curved edge.
5. Move the cube a few metres and let go. The pink follows it, and nothing is left where it stood. Delete
   the cube: the floor goes back to plain. Undo: the pink comes back.
6. Change the bounce strength 2 → 5 → 2. The floor ends up exactly as it was at the start.
7. Do steps 2 and 5 on the `Hill` landscape from 082, on a slope. The same holds.
8. Fly around the cube and away from it and back. The tint stays put on the ground, does not pop or
   crawl, and the editor stays smooth.
9. Press Play: the tint is the same in the game window.
