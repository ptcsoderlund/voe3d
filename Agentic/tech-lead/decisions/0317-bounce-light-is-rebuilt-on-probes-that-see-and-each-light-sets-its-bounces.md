# 0317 — Bounce light is rebuilt on probes that see, and each light sets its own bounces
date: 2026-10-02
by: tech-lead

## Decision
046's bounce is not good enough to keep: flat ground lit only by the sun shows blotches that move as the sun
turns, because the ground lights itself through a 2 m grid and a gain of 10 magnifies every error (0311,
0313–0315 tuned it against one scene). It is rebuilt, in work order 051:
1. **Each light sets its bounces**, 0 by default (0316). The sun may want 2, a lamp 0 or 1. The highest
   allowed is 3. Bounces are per light, not per scene.
2. **Probes that see.** Each probe keeps a small all-round picture of what surrounds it, drawn by
   rasterising the scene as any view is drawn, and its distances, so light does not pass through walls or
   into shadows. No ray tracing and no distance fields (0307 point 8 stands).
3. **Relit, not redrawn.** A probe's picture is drawn again only when something near it moves. Each update
   relights the pictures from the lights' shadow maps and from the previous bounce; each pass is one more
   bounce, carried for each light only as far as that light's count.
4. **Semi-baked.** When the bounce has settled and nothing moves or changes, no picture is drawn and no
   relighting runs: a lit pixel only reads the result, close to the cost of baked light. A moving light (a
   day cycle) or moving thing keeps only the part of the work it touches running.
5. **No gain.** Bounce strength is honest light, shown at the strength a light gives it; a look that needs
   more is a per-light bounce strength, not a hidden constant.
6. **Order.** First 047 makes 046's bounce opt-in per light. Then 048–050, whose batched six-view drawing
   of point-light shadows (0301) is the same drawing the probes need, built once. Then 051 replaces 046's
   bounce.

## Reasoning
The sponsor wants a bounce to be proud of, a bounce count per light, and the cheapest possible cost once
settled. A per-light count fits this design at a cost of memory per distinct count in use, not per light.
- Patch 046's grid (opt-in, no self-lighting, no gain): leaks stay, several bounces do not fit.
- Remove the bounce, keep the fill: honest but no path forward.
- Ray-traced or distance-field probes: ruled out by 0307 for good.

## Replaces
0307 points 1–4, 0308, 0310, 0311, 0313, 0314 and 0315 once 051 is built; until then they describe 046's
bounce. 0307 points 5–8 and 0312's rule stand.
