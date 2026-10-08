# 05 — A small coloured box does not tint the ground beside it

## Seen
On Windows, in the editor, with the sun at bounces 2. Three screenshots are beside this file.

- `05-purple-cube-on-landscape.png`: a 1 m purple cube on the landscape, bounce strength 2. The sun is low
  and the cube's left face is in full sun, bright magenta. The ground in front of that face shows no
  purple.
- `05-purple-cube-and-red-slab-on-landscape.png`: the same with bounce strength 5 (sun intensity 3.142,
  fill 0.216), and a thin red slab on the ground in front of the lit face. Still no pink on the ground
  beside the cube, and no visible red from the slab on the cube's face.
- `05-purple-cube-on-orange-floor.png`: no landscape in the scene. The same cube on a stretched orange box
  as a floor. The floor beside the lit magenta face still takes no pink. A faint curved edge runs across
  the floor at the bottom right, a line where the floor's shade changes, with nothing in the scene to
  cast it.

What does work: the landscape's hill gets brighter with bounce on and darker with it off (the flat ground
lights it), and the cube's faces pick up the ground's colour. Light from a large surface bounces; light
from a small one does not show.

## Expected
Spec 051, how to test step 3: beside a strongly coloured box in sunlight, the ground on its lit side takes
the box's colour, fading with distance. And step 2: flat ground is even, with no blotches or streaks, so the
curved edge should not be there.

The landscape is not fine until this works: the hill's world needs things on it to colour the ground
around them.

## How to reproduce
1. On Windows, build the debug preset and start the editor.
2. A scene with a flat floor (a stretched box is enough), no landscape, and a sun at bounces 2, bounce
   strength 2 to 5, low in the sky.
3. Put a 1 m cube in a saturated colour (purple, red) on the floor.
4. The floor beside the cube's sunlit face does not take its colour.

Whether this ever worked (051 was accepted on `examples/tank_game`), and whether a bigger box tints the
ground where a 1 m one does not, are not known.
