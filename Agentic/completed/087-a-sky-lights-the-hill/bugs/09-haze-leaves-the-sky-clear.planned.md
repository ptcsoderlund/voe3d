# 09 — Haze leaves the sky clear

## Seen
The sponsor: "it is now close and very good. I would like the haze to also affect the sky. If i have dense fog
with close haze, everything is foggy except the sky which is clear as if there is no fog."

## Expected
- Haze has a height: how high the haze layer reaches.
- The sky is seen through the haze. Dense, close haze turns the whole sky to haze. Light haze makes the horizon
  hazy, and the sky gets clearer the higher you look. Above the haze layer, looking up, the sky is clear.
- A sun's disc and glow fade with the sky: visible through thin haze, gone in dense fog.
- With Haze and no Sky, the backdrop fades into the haze the same way.
- The ground still fades by distance, as now. Without Haze the sky does not change.

Decided in 0418.

## How to reproduce
1. Open lumberhill_project in the editor, with a Sky, a directional light and Rendering / Haze.
2. Set the haze close and dense (small haze, small start): the hill disappears into fog, but the sky and the
   sun's disc stay clear.
