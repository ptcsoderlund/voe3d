# 01 — Light passes through a blocker, and every blocker is a Room

## Seen
"A light blocker should break the light. If a sun hits a lightblock and the lightblock is in the air, the sun
should not light up the other side of the block in its direction. It will look like it is casting shadows,
but in reality they are blocking light. Right now, it's lacking." A blocker only keeps light out of its own
inside; light passes straight through it to whatever is behind. And with only one kind of blocker, a house
cannot let the sun in through its windows while its walls keep it out.

## Expected
As decision 0348 says:
- No direct light passes through any blocker: not a directional light, not a point light with Cast shadows
  off, not bounce. What is behind a blocker, seen from a light, gets none of that light. A blocker floating in
  the air leaves a shadow-shaped patch on the ground in the sun's direction, with fill in it like a real
  shadow.
- Fill is not stopped by passing a blocker. It is only kept out of Indoors and Room boxes.
- Each blocker has a Kind in the editor, saved with it:
  - **Wall**: stops direct light, nothing more.
  - **Indoors**: no outside fill inside it. Direct light comes in only where no Wall stops it, and lights
    inside leak out only the same way.
  - **Room**: as 056 built it. Nothing from outside gets in, and nothing from inside gets out, including a
    directional light placed inside it, which lights only that Room in its own direction.
- A new blocker is a Room, and blockers saved before this keep their look. A scene with no blockers looks
  exactly as before.

## How to reproduce
1. Open a scene with a directional light casting no shadows and a flat ground. Add a blocker, kind Wall, and
   raise it into the air. The ground below it, in the sun's direction, gets a dark patch the shape of the box.
   The patch is not black: the fill shows in it. Move the sun: the patch moves with it. Move the blocker: so
   does the patch.
2. Put a point light with Cast shadows off on one side of a Wall blocker standing on the ground. The ground on
   the far side of the blocker gets none of its light; to the sides it does.
3. Open `examples/tank_game`. Line a house's walls with Wall blockers placed just outside its inner walls,
   leaving its windows and door open, and put one Indoors blocker over its inside. Inside the house it is
   dark except where the sun comes through a window or the door. With the sun's Bounces at 1 or more, that
   sunny patch softly lights the room. The outside of the house, its back included, looks as it did before:
   lit, and with fill where the sun does not reach.
4. Put a point light with Cast shadows off just outside a wall of that house. Its light reaches inside only
   through a window or the door. Put a point light inside the house: it lights the room and shows outside
   only through the window and the door.
5. Make a cave: a Room blocker around a space, entrance and all. Nothing from outside lights it, not through
   its entrance either. Put point lights inside as crystals: they light the cave and never show outside.
   Move the scene's directional light into the Room: it lights only the cave, in its own direction, and the
   outside goes dark (until several directional lights arrive in 058).
6. Change a blocker's Kind, save, close and reopen: the kind is kept. Open a scene with blockers saved before
   this: they are Rooms and look as they did. Open a scene with no blockers: it looks exactly as it did.
7. Press Play in each case above: the game looks the same as the editor, and the blockers are invisible.
