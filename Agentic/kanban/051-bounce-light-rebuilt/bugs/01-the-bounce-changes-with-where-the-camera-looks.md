# 01 — The bounce changes with where the camera looks

## Seen
"Now when I look in the room it is lit with bounces. When I look to the side and the room entrance is on the
right side of the screen, it darkens. As if it's screen-spaced bounces. I thought our probes were realtime
baking, it should not flicker or change based on where camera is."

## Expected
The bounce belongs to the world, like baked light. Turning the camera, or looking away and back, never changes
how bright the room is or makes it flicker. Only a light or a thing that moves changes the bounce, and only
around itself (051 point 8). No screen-space lighting (0328).

## How to reproduce
1. Open a scene with a house that has a doorway and a light giving Bounces 1 or more, so the inside of the
   house is lit by bounce.
2. Stand outside, or in the doorway, and look into the room: the room is lit by the bounce.
3. Turn the camera without moving it until the doorway is at the right edge of the screen: the room darkens.
4. Turn back: it lightens again. Repeat with the camera standing still, only turning; the room should look the
   same at every angle.
