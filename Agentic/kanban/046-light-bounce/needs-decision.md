# 046 — needs decision: how the bounce is found

0268 milestone 13 says the bounce technique is chosen by its own decision, and the ideas list leaves
its core open: who sees whom. The answer reaches past this feature — it can set a GPU capability tier
(0063), a project `Cache/` folder, and the shape every later light (048's point lights, 050, a sky)
feeds into — so the planner does not make it.

## Question
How does a sunlit surface find what it lights, at run time, with no bake step?

## Options
1. **Reflective shadow map into a cached probe grid, no visibility.** The sun's shadow pass also
   writes each texel's received light × base colour; those texels are the "lit surfaces become
   lights". A compute pass gathers them into a coarse 3D grid of irradiance probes around the
   camera, a slice per frame (whole cycle ~50 ms), blended over frames; every lit draw samples the
   grid where 0275's fill now sits. Any Vulkan 1.3 card, no new tier, nothing on disk, moving things
   lit for free. Cost: bounce is not occluded, so it can leak through thin walls (the
   light-blocking volumes of the ideas list are the later fix).
2. **A distance field of the world, traced in compute.** Each mesh gets a signed distance field,
   built automatically on load and kept in a project `Cache/`; probes or a surface cache trace it
   for visibility. Correct occlusion, Lumen's shape. Cost: a new on-disk cache convention for every
   project, mesh-to-field building, a much larger feature.
3. **Hardware ray queries.** Acceleration structures over the geometry pools, rays from probes.
   Exact, simplest shaders. Cost: a ray-tracing tier under 0063, which still requires a baseline
   path (so option 1 anyway), and pool freeing (D-068) pulled forward.

## Also open with it
Whether 0273's fill light stays as a floor under the bounce or is replaced by it (the feature's
"instead of by a flat fill" reads as replaced in shade the bounce reaches).

## Recommendation
Option 1. It is the baseline 0063 demands of any later option, it fits a sunny top-down level where
thin-wall leaks rarely show, it settles within the feature's second, and it keeps the tank game
smooth. Keep the fill as a floor under the bounce, default 0 in new scenes, so dark corners the
grid misses are not black. Options 2 or 3 can follow as a tier when a game needs occlusion.
