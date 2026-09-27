# 043 — Several lights

## What
A scene can have point lights beside the sun: a colour, a strength and a reach each. They are placed,
parented and seen in the editor like anything else, shown by a marker. A level can hold a hundred of
them and stay smooth. Game code can light one for a moment, so shots and explosions flash light onto
the ground around them. Point lights do not cast shadows in 0.2. The sun still does. The editor and
the game look the same.

## Why
Milestone 10 of 0268. Lamps along the level, and flashes that light the world at every shot.

## How to test
1. In `examples/tank_game`, add a point light near the ground at dusk (a dim orange sun). A pool of
   light shows on the ground. Change its colour, strength and reach. The pool follows live.
2. Place a hundred lamps along the level by duplicating. The editor stays smooth.
3. Parent a light to the tank's turret. Moving the tank carries the light.
4. Play. Each shot flashes the ground around the barrel. Each explosion lights up its surroundings
   for a moment.
5. The lamps look the same in the game as in the editor.
