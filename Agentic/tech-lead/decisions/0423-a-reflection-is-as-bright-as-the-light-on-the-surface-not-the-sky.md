# 0423 — A reflection is as bright as the light on the surface, not the sky
date: 2026-10-13
by: tech-lead

## Decision
For 088 bug 01. Only lights light, and that now includes reflections. Changes to how 0411 point 5 works:

1. **A shiny surface still mirrors the sky.** Its colours, the lights' discs and their glows all show in the
   reflection, gated by blockers as before.
2. **The reflection's brightness comes from the light reaching that surface.** That is the same light that
   lights its colour there: the directional lights where their shadows let them through, their fill, point
   lights and bounce.
   - In full, unshadowed light, a mirror shows the sky no brighter than the sky's own colours.
   - Less light gives a dimmer reflection.
   - A surface no light reaches reflects nothing, so a metal cube in shadow with no fill is dark.
3. **The sky's strength no longer changes any surface.** It scales the backdrop and the haze, as 0411
   points 3 and 4 say, and nothing else. Turning sky strength from 0 to 1 leaves every model, shape and the
   terrain exactly as they were.
4. The planner picks how the light is measured and how it scales the reflection. It must not reintroduce
   the sky as a light in any other way.
5. No Sky row still means the old picture, byte for byte (0411 point 7).

## Reasoning
The sponsor tested 088 with a wooden stable and a metal cube. Turning sky strength from 0 to 1 lit their
shadowed sides from nearly black to clearly lit. The terrain did not change, because it is rough enough to
barely reflect. That reads as the sky lighting the world, which 0411 point 1 forbids: "the reflection's
intensity should come from the light, not the sky." Alternatives:
- **Keep 0411 point 5 as it was:** this is what the sponsor tested and rejected.
- **Fade the reflection out on rough surfaces only:** the wood would be fixed, but a metal cube in shadow
  would still glow with the sky.
- **Drop sky reflections:** chrome reflecting black looks broken.

## Replaces
Amends 0411 point 5: the reflection stays, its brightness is the light's. Amends 0411 point 3: sky strength
scales only the backdrop and haze, never a reflection.
