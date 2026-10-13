# 06 — Haze lights up the world

## Seen
After cards 33–50, testing 087: "Almost, when haze is not 0 it lights up the environment. Which we dont want.
Only light should light up."

With haze above 0, the scene gets brighter: surfaces read as lit by the haze, not only by the lights.

Asked whether this is 0411 point 4's far fade, the sponsor answered: "It's not the far fade, the whole scene
brightens." Near surfaces brighten too, so this is a bug, not the design.

## Expected
Only lights light the world (0411 point 1: "Lights are responsible for all lighting, and the sky only for the
haze"). Haze must not make the world brighter as if it were a light.

0411 point 4 still holds: far surfaces fade into the sky's horizon colour times the sky's strength. If what the
sponsor sees is that fade, applied as 0411 has it, then the fade itself is what they reject. The planner then
stops with needs-decision and does not change the haze on its own. If the haze reaches further than "far" or
adds light rather than blending, that is the bug.

## How to reproduce
1. Open lumberhill_project in the editor, with the sky on.
2. Set the sky's haze to 0 and look at the ground, the cube and the slab.
3. Raise haze above 0 and watch the scene brighten.
