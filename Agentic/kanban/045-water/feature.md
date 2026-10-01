# 045 — Water

## What
A water surface can be placed like any other thing and sized to cover an area. It moves in small
waves, catches the sun as glints, reflects the sky's colour, and gets darker and less see-through
the deeper it is, so the shore shows through at the edges. It is lit and shadowed by the scene and
looks the same in the editor and the game. Its colour and wave size are set in the Inspector. In the
tank game, water runs along both sides of the level.

## Why
Milestone 12 of 0268. The level is a strip of land between two stretches of water.

## How to test
1. In `examples/tank_game`, place a water surface over a lowered stretch of ground. It moves in the
   editor views.
2. Turn the sun. The glints move across the water.
3. Look at the shore. The water is clear at the edge and dark further out.
4. Change the colour and wave size in the Inspector. It changes live, and undo works.
5. Cast a tank's shadow onto the water. The shadow shows.
6. Play, with water along both sides of the level. It looks as it did in the editor, and the game
   stays smooth.
