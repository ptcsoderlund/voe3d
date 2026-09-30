# 050 — Point lights cast shadows

## What
A point light has a "Cast shadows" checkbox, off by default. When it is on, things near the lamp throw shadows away
from it in every direction, and meshes with their own Cast shadows off (049) throw none. Many shadow-casting lamps
are drawn in batches, not one extra pass per lamp, so the level stays smooth. When there are more than a set number
of shadow-casting lamps, the ones that matter most for the view cast shadows and the rest light without them, with
no popping as the camera moves. See 0301.

## Why
Lamps along the level should make things cast shadows in the dark, without a hundred lamps costing a hundred passes.

## How to test
1. Open `examples/tank_game` at dusk (a dim orange sun) with a lamp beside a house. Tick the lamp's Cast shadows.
   The house throws a shadow on the ground away from the lamp. Move the lamp around the house: the shadow swings
   to the opposite side.
2. Drive the tank between the lamp and a wall. The tank's shadow falls on the wall.
3. Untick Cast shadows on the house (049). Its shadow from the lamp goes too.
4. Duplicate lamps with Cast shadows on until there are a hundred along the level. The editor and the game both
   stay smooth.
5. Fly the camera along the lamps. Shadows near the view are there. No shadow pops in or out close to the camera.
6. Play. The lamps' shadows match what the editor showed.
