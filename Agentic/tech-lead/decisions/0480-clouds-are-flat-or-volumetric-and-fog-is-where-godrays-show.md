# 0480 — Clouds are flat or volumetric, and fog is where godrays show
date: 2026-10-25
by: tech-lead

## Decision
1. Clouds are their own scene row with a Kind, Flat or Volumetric: "Some games we want flat, but in this
   demo we want volumetric." A Flat cloud is a drifting layer at one height. A Volumetric cloud has depth:
   it can be flown up to, and it looks different from every side. Both kinds have coverage, height, wind
   direction and speed. Volumetric clouds also have thickness.
2. Both kinds cast shadows on everything below them, the same clouds that are seen in the sky. How dark a
   cloud's shadow is comes from the cloud row's own Shadow strength, not from the light's fill: "Was
   thinking about "fill" we have in directional lighting but a separate one for the clouds." The light's
   fill still sets how dark the shadows of objects are.
3. The directional lights light the clouds (0411), so a low, orange sun colours them. The sky stays the
   authored gradient of 0411, and clouds are the only thing added to it: "I think clouds is the only thing
   we need for a proper sky. Devs have control of colors and stuff, they program night/day cycle and make
   colors and such by hand." There is no physical sky and no built-in time of day.
4. Haze stays as it is: "Haze is also consistent and regular fog." Haze is everywhere; fog is where the
   developer puts it. Fog is a fog volume placed in the level like any entity: a box moved and sized with
   the gizmos, with thickness, patchiness and wind. Inside it the fog is thick in patches and thin between
   them, drifting and changing shape irregularly, and it fades out softly towards the box's edges, so no box
   shape shows. Outside every fog volume there is no fog, only haze. There is no world-wide fog row.
5. Godrays are sunlight seen in the fog, with shadow wherever clouds, trees or buildings block the sun.
   Each fog volume has a Godrays strength: beams show inside fog, whether or not the sun is on screen. A
   large, thin, even volume over the valley gives beams there without visible patches; beams through cloud
   gaps show only where such a volume is.

## Reasoning
A Kind lets one engine serve both a cheap game and this demo. Clouds that cast shadows from what is seen
in the sky never disagree with it. Placed fog keeps fog apart from haze, and the sponsor places
everything on the hill by hand (0376). Godrays taken from the fog fall between the trees and through
cloud gaps alike, and stay true when the sun is behind the camera.
Alternatives considered:
- Screen-space godrays smeared out from the sun's disc: rejected, because they vanish when the sun leaves
  the screen and ignore the forest.
- A physical sky with time of day: rejected, because the sponsor makes day and night by hand.
- One world-wide fog row lying low up to a height: rejected after the sponsor asked how it differs from
  haze; it would roll over the meadow and round the house as well as through the forest.
- Both a fog row and fog volumes: rejected as two ways to do one thing.
- Godrays in the haze too, taking shadows with its own strength: left out for now. The sponsor: "We
  leave haze as it is now. We probably revisit this later." It is an idea.

## Replaces
Nothing. Extends 0411 and 0416.
