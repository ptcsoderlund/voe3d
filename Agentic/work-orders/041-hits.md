# 041 — Hits

## What
A game's code can ask what a fast thing would hit on its way: a ray, or a sphere swept along a line,
answered with the first thing hit, where, and the surface's direction there. This sits beside 0253's
overlap. It is fast enough for hundreds of shells a frame, and never misses a thin wall because a
shell moved past it between two steps. In the tank game, a shell stops where it hits. Hitting an
enemy or a piece of scenery that can be destroyed swaps in its wreck, the way 0268 fakes destruction.
Hitting the player's tank costs it a life.

## Why
Milestone 8 of 0268. Shells move far too fast for overlap tests to catch.

## How to test
1. Play the tank game. Fire at an enemy tank. The shell stops on it, and the tank turns into its
   wreck.
2. Fire at a wall at point-blank range, and at a thin post from far away. The shell never goes
   through either one.
3. Fire at a destructible house. It becomes its wreck. Fire at the wreck. The shell stops on it, and
   nothing changes.
4. Let an enemy shoot you. The tank loses a life.
5. Hold fire into a crowd of enemies for a minute. The game stays smooth.
