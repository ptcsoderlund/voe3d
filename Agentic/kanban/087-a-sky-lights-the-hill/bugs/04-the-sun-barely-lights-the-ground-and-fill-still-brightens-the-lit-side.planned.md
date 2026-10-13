# 04 — The sun barely lights the ground, and fill still brightens the lit side

## Seen
After cards 26 and 27, in the editor built from them: "No not really." "Only changes i made are
fill_intensity." "Knowing the sun is not lighting should be info enough."

lumberhill_project, sky on, the sun low (the sky pink towards it). The Light: colour a light pink, intensity
3.142, fill colour blue, bounces 1, cast shadows true, bounce strength 2.0.
- Fill intensity 0.001: the whole view is nearly black. The ground in the sun is a dark brown, the red cube
  near black, the slab's sunlit top a dim pink. The sun on its own hardly lights anything.
- Fill intensity 0.562, nothing else changed: everything brightens together. The ground turns green, the
  cube purple, and the slab's sunlit top goes from dim pink to pale peach. The lit side brightens as much as
  the shade, which is bug 02 still there.

Card 26's test reads a face in full sun at 186 186 186 for fill 0, 0.5 and 1, so the test does not draw what
the editor draws.

## Expected
- The sun lights what faces it: with fill at 0, a surface the sun reaches reads clearly lit in its own
  colour, not near black, the low sun included.
- Fill lights only what the sun does not reach, as bug 02 asks: a surface in full sun reads the same
  whatever the fill strength.
- The test for this draws the case the editor does (the sky on, a low sun, the hill's light settings),
  so that it fails on the code as it is now.

## How to reproduce
1. Open lumberhill_project in the editor, with the sky on and the sun low.
2. Set the Light's fill intensity to 0.001 and look at the ground, the cube and the slab in the sun.
3. Raise fill intensity to about 0.5 and watch the sunlit slab top and ground brighten.
