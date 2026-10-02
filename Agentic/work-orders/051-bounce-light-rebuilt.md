# 051 — Bounce light, rebuilt

## What
046's bounce is replaced (0317). Every light, the sun and point lights, has Bounces from 0 to 3 and a bounce
strength, Bounces 0 by default (0316). Light bounces off lit surfaces onto what faces them, taking their
colour, and each extra bounce carries it one surface further: a sun at 2 lights the back of a room through
its doorway more than at 1. Flat ground lit by the sun is even, with no blotches or streaks, at any sun
angle. Bounce does not pass through walls and does not lighten a shadow from the face that casts it. Once
nothing moves, the bounce settles and stops costing more than reading it, like baked light; a moving light or
thing updates only the bounce around it. It runs smoothly on the cards 0318 names. Scenes that set Bounces 1
under 047 keep it. The game looks the same as the editor.

## Why
After 048 the sponsor finds the bounce looks good, with no smudges, so this is not a rescue: today's look at
Bounces 1 is the floor. 046's bounce cannot go past one bounce; the sponsor wants up to 3 per light and the
light's quality raised a step above today's, cheap once settled.

## How to test
1. Before building, save a screenshot of `examples/tank_game` with the sun at Bounces 1. After: with the
   same scene and the default bounce strength, the picture looks at least as good, with the bounce about as
   bright, and clearly a little better where bounce shows: softer, cleaner colour near walls and boxes.
2. Open `examples/tank_game` and set the sun's Bounces to 1. Open flat ground is evenly lit, with no blotches
   or streaks. Turn the sun slowly: the ground stays even while its brightness follows the sun.
3. Beside a strongly coloured box in sunlight, the ground on its lit side takes the box's colour, fading with
   distance. The ground in the box's shadow does not take it.
4. Build a closed box room in sunlight. Its inside stays dark: no bounce comes through the walls. Cut a
   doorway: light comes in through it.
5. Set the sun's Bounces to 2, then 3. The room's far corners, which the doorway does not face, get lighter
   at each step. Back to 0: the room is as it was before 046.
6. Raise and lower the sun's bounce strength: the bounce grows and fades, the direct sunlight does not change.
7. At dusk (a dim orange sun), give a lamp Bounces 1 near a white wall. The wall around the lamp's pool takes
   the light that bounced off the ground. Set it back to 0: only the pool remains.
8. Stop touching anything. Within a moment the picture stops changing and the laptop stays calm. Drive the
   tank: the bounce around it follows it, and the rest of the level does not flicker.
9. Open a scene saved under 047 with the sun at 1: it is still 1.
10. Play. The game's bounce matches the editor's.
