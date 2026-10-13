# 0418 — Haze reaches a height and veils the sky behind it
date: 2026-10-13
by: tech-lead

## Decision
For 087 bug 09. The sponsor: "I would like the haze to also affect the sky. If i have dense fog with close
haze, everything is foggy except the sky which is clear as if there is no fog." Asked to choose, they chose
"haze with a height" and "disc should fade".

1. **Haze has a height.** The Haze row gains `height` after `start`: the world height (metres, y up) that the
   haze layer reaches up to. It is any finite number, and its default and unsaid value is 1000. Surfaces fade
   by distance only, as 0414 and 0416 say. `height` does not change how surfaces fade.
2. **The sky is veiled by the haze it is seen through.** For a view direction with elevation `θ` from a
   camera at height `y`, the path through the layer is `L = (height − y) / sin θ` when the camera is below the
   top and looks up. With the camera inside the layer looking level or down, the path is endless, and so is
   it for a camera above the top looking down. With the camera above the top looking up, the path is 0.
   The sky's colour fades towards the haze colour by 0414's fade `f = 1 − 2^(−max(L − start, 0)/haze)`,
   with an endless path giving `f = 1`. The haze colour is 0416's. So dense, close haze turns the whole sky
   to haze, and light haze turns the horizon to haze and leaves the zenith nearly clear. The horizon meets
   far ground, which also fades fully, without a seam.
3. **The discs and glows fade with the sky behind them**, by the same `f` along their direction. The sun
   shows through thin haze and is gone in dense fog.
4. **With a Haze row and no Sky row**, the backdrop behind everything fades the same way, from what is shown
   there now towards the haze colour.
5. **Reflections keep the unveiled sky.** A shiny surface hazes by its own distance, as any surface does.
6. No Haze row means no change to the sky. No Sky, no Haze and no Picture row still mean the old picture, byte
   for byte. The tank and coin games have no haze and do not change.

## Reasoning
- **A height, against one sky distance:** a single distance fades the zenith as much as the horizon, so a
  thin valley haze flattens the whole sky. A layer gives real haze's look: a hazy horizon under a clear
  zenith, a grey sky in fog, and a clear sky above the fog once you climb above the layer.
- **Against "the sky is infinitely far, so fully hazed":** any haze at all would erase the sky.
- **Height on the sky only, against height fog on surfaces too:** fog that pools in valleys is a feature of
  its own (ideas.md). Here the ground fades as it did.
- **Discs fade:** a sharp sun disc through dense fog looks wrong.

## Replaces
Amends 0416 point 1 (the Haze row gains `height`) and 0417 point 3 (its fields). 0405's and 0411's sky now
fades behind haze.
