# 02 — Cast shadows decides whether a directional light inside a blocker lights it

## Seen
"If I place a directional light inside a blocking volume, it only works if cast shadows is false. Else the
volume stays black." "Every type of block has a problem. With fill blocked it's inverted."

With Block All and Block Direct, a directional light inside the blocker lights the inside only while its Cast
shadows is off; turn it on and the inside goes black. With Block Fill it is the other way round: on lights the
inside, off leaves it black.

## Expected
A directional light placed inside a blocker is active within that blocker, of every Block kind (0348, 0352):
it lights the inside in its own direction, with its fill, whether Cast shadows is on or off. Cast shadows only
adds shadows: with it on, things inside the blocker cast shadows from that light, and the inside is otherwise
lit as with it off. Lights outside the blocker behave as today.

## How to reproduce
1. Open `examples/sun_and_moon`, or any scene with a sun. Add a light blocker, Block All, and size it around a
   floor with something standing on it.
2. Add a second directional light and place it inside the blocker. Cast shadows off: the inside is lit by it.
3. Turn its Cast shadows on: the inside goes black. Expected: still lit, and the thing on the floor casts a
   shadow.
4. Repeat with Block Direct: the same as Block All.
5. Repeat with Block Fill: Cast shadows on lights the inside and off leaves it black. Expected: lit both ways.
