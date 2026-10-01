# 0307 — Bounce light is a VRAM probe cache fed by the shadow map and refined over time
date: 2026-10-01
by: tech-lead

## Decision
For 046, and the shape every later light feeds into:
1. **What lights what.** Each casting light's shadow pass also writes, per texel, the light it received × base
   colour (a reflective shadow map). Those texels are the lit surfaces that become lights. No visibility is
   traced: bounce is not occluded and may leak through thin walls.
2. **A cache in VRAM, not a bake.** The result lives in a coarse 3D grid of irradiance probes around the camera,
   kept in VRAM across frames. Nothing is written to disk; there is no bake step and no project `Cache/`.
3. **Updated slowly, over time.** A compute pass refreshes the grid a slice at a time each frame and blends new
   results into the old, so the cost per frame is small and fixed. A light, or a lit thing, that moves makes
   the probes it touches go stale; stale probes are refreshed first, the rest keep cycling. The grid settles
   within the feature's second. Think of it as a semi-baker: a baked look, kept current at run time.
4. **Read by every lit draw** where 0275's fill now sits.
5. **The fill stays as a floor** under the bounce: it still lifts shade the bounce leaves dark, default 0 in new
   scenes, existing scenes keep their value. The sponsor rates the fill above Unity's and Godot's ambient; it is
   kept, not phased out. Amends 0275.
6. **Any Vulkan 1.3 card.** No new capability tier under 0063. This is the baseline any later tier must keep.
7. **Later, not in 046:** hand-placed volumes that block light or limit reflections, which can be children so
   they follow their parent's transform. Some manual work for the game's makers, cheap at run time. A volume
   blocks everything a light brings in, the fill included: a windowless house in a sunlit world is black
   inside until a light is placed there. That is how inside/outside transitions are made.
8. **No ray tracing, no distance fields, no physically based global illumination.** VOE3D makes games that look
   like games: cheap, fun cheats with high performance over the laws of physics. Occlusion of light comes from
   volumes the game's makers place, never from tracing the world.

## Reasoning
The sponsor wants lighting that caches in VRAM and refines asynchronously over time, cheap on the shipped game,
with manual volumes for the cases leaks matter. The reflective shadow map into a probe grid is exactly that,
runs everywhere, and is the baseline the other options would need anyway.
- A distance field traced in compute: correct occlusion, but a new on-disk cache for every project and a much
  bigger feature. Rejected for good.
- Hardware ray queries: exact, but a new GPU tier and physical realism the product does not want. Rejected for good.

## Replaces
Nothing. Amends 0275 (the fill now sits under the bounce).
