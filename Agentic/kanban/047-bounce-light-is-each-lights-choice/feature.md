# 047 — Bounce light is each light's choice

## What
The sun has a "Bounces" setting in the Inspector: 0 or 1 for now (051 raises the limit). A new scene's sun
and any new light start at 0 (0316). At 0 the light's bounce costs nothing at all and the scene looks as it
did before 046: the fill light still lifts the shade, and flat ground lit by the sun is flat, without
blotches. At 1 the sun bounces as 046 built it. Every scene, the ones saved before this change included,
starts at 0, since the bounce was never saved. The setting is saved with the scene and the game looks the
same as the editor. When no light bounces, the engine does no bounce work at all.

## Why
0316: performance is the default and every costly look is opt-in. 046's bounce is always on and does not look
good enough to be on by default; 051 rebuilds it.

## How to test
1. Start a new scene. Select the sun: Bounces shows 0. The ground at the foot of a coloured box has no tint.
2. Open `examples/tank_game`. The sun's Bounces is 0. The ground is evenly lit, with no blotches, and turning
   the sun changes how bright the ground is but leaves it even.
3. Set Bounces to 1. The ground beside a coloured box takes its colour, as in 046. Set it back to 0: the tint
   goes.
4. Undo and redo the change: the tint comes and goes each time.
5. Set 1, save, close and reopen the project. Bounces is still 1.
6. Play. The game shows the bounce exactly when the editor did.
