# 056 — Light-blocking volumes

## What
A Light blocker is a box you add in the editor, sized and placed by its transform and parentable like
anything else (so it can ride with a house prefab). Light from outside it does not get in: not the directional
light, not its fill, not a point light outside the box (cast shadows on or off), and not the bounce. Lights
inside the box still light the inside, and their bounce stays inside. Nothing outside the box changes. You can
see the box in the editor as an outline when it is selected. In the game it is invisible. It casts nothing,
collides with nothing and costs close to nothing, so you can use it where shadows would be too costly (0316).
With no blockers, a scene looks exactly as before. This is the light-blocking volume 0307 promised.

## Why
The inside of a house is bright from the fill, and a lamp outside lights the inside through the wall when it
casts no shadows. Shadows everywhere are too costly; a box that says "outside light stays out" is the cheap
cheat.

## How to test
1. Open `examples/tank_game`. Walk into a house: its inside is bright from the fill, as today.
2. Add a Light blocker to the house and size it to the house's inside. The inside goes dark: no fill, no
   sunlight, no bounce from outside. Outside the house nothing changes.
3. Put a point light outside the house with Cast shadows off, near a wall. Before the blocker its light shows
   inside through the wall; with the blocker it does not, with Bounces 0 or 3.
4. Put a point light inside the house. It lights the room, and with Bounces 1 or more its bounce fills the
   room. None of it shows outside through the walls.
5. Make the house a prefab with the blocker as a child. Spawn two and move one: each blocker follows its house.
6. Deselect the blocker: no outline. Press Play: the blocker is invisible, the shell and tank pass through it,
   and the inside looks the same as in the editor.
7. Save, close and reopen: the blocker is still there, with its size.
8. Open a scene with no blocker, saved before this: it looks exactly as it did.
