# 027 — Stand, walk into walls, jump and touch coins

## What
Things in a level can be solid. In the editor I add a collider to an entity (box, sphere or
capsule), see its shape drawn on the selected entity, and set its size in the Inspector. A new
collider starts out fitting the entity's shape. A collider can be marked as a trigger, which
nothing bumps into but which notices what touches it.

The capsule gets a kinematic body. In the game it stands on the floor, falls off edges, stops
at walls and slides along them, walks up a gentle ramp but not a steep one, steps up onto a low
ledge without jumping, and jumps with Space. Gravity and the jump are the example project's own
code, with a jump height and gravity I can set in the Inspector. Touching a coin that is a
trigger makes it disappear, which is the example project's code too.

The example project `examples/capsule/` is updated to show all of this: a level with a floor,
walls, a gentle ramp, a steep ramp, a low ledge, a gap to fall through, and a few coins. See
decisions 0249 and 0250.

## Why
Milestone 4 of 0186: without collision there is nothing to stand on and nothing to collect.

## How to test
1. Open the editor and open `examples/capsule/`. The level has a floor, walls, two ramps, a low
   ledge, a gap in the floor and some coins.
2. Select the floor. It has a box collider, drawn as lines around it in the scene view. Change
   its size in the Inspector and the lines follow. Undo puts it back.
3. Add an entity with a cube shape, then add a box collider to it. The collider's lines fit the
   cube. Scale the entity and the lines scale with it.
4. Select a coin. Its collider is marked as a trigger.
5. Press Play. The capsule stands on the floor and does not sink or jitter.
6. Walk into a wall head-on: the capsule stops. Walk into it at an angle: it slides along it.
7. Walk up the gentle ramp: the capsule goes up. Walk into the steep ramp: it does not climb it.
8. Walk into the low ledge: the capsule steps up onto it without jumping.
9. Press Space: the capsule jumps and lands. Pressing Space in the air does nothing.
10. Walk into the gap: the capsule falls through it.
11. Walk into a coin: it disappears. You walk through it rather than bumping into it.
12. Stop. In the editor, raise the capsule's jump height and press Play again. It jumps higher.
13. Remove the collider from one wall and press Play. You walk through that wall. Undo, Play,
    and the wall stops you again.
14. Save, close and reopen the editor and the project. Every collider, trigger and value is as
    you left it.
15. The capsule moves as smoothly as before, whatever the frame rate, with no stutter.
16. In the editor, move every entity 100 km along X (type 100000 added to each X in the Inspector)
    and press Play. Standing, walls, ramps, the ledge, jumping and coins all behave exactly as in
    steps 5–11, and nothing shakes. In the editor the objects do not shake either, up close.
