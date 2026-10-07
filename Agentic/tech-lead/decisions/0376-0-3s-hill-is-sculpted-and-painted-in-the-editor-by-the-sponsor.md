# 0376 — 0.3's hill is sculpted and painted in the editor, by the sponsor
date: 2026-10-07
by: tech-lead

## Decision
0.3's road, which 0374 left open. The hill is a terrain the editor owns. It is sculpted with brushes
(raise, lower, smooth, flatten) and its ground is painted with layers of materials (dirt, grass, moss,
rock), all in the editor. Grass grows from the painted grass layer, and trees and rocks are painted
onto the terrain with a foliage brush. Models (house, tower, trees, rocks) come from Blender or are
downloaded, as the sponsor chooses, and arrive as `.glb` with their licences. Every piece of content in
the world is made by the sponsor's hand. An agent builds the tools and never authors the hill, a scene,
a material or a placement, and a work order's test is something the sponsor does in the editor. The road
is work orders 081–091, in this order: the terrain and its brushes, the terrain to the horizon, editing
materials, painting the ground, a sky, models looking as they do in Blender, LOD levels, painting a
forest, grass, compressed textures, and the hill shipped. Work orders 067–080 are withdrawn. What they
held that the hill needs is rewritten: environment light and a tone curve into 085, textures read the
way the file says and surfaces into 086, instancing (0375) into 088 and 089, texture compression into
090. The rest of glTF's breadth waits until a game needs it.

## Reasoning
The sponsor's call (2026-10-07): "C, lets do it properly. Or else its gonna be bad workflow and hard to
do foliage." Grass and trees are placed by reading and painting the ground. That only works well when the
ground is the engine's own and is painted on the picture the game shows (0375 point 5). "Everything is in
my hands, no ai do any editor work": the project is the sponsor's road to learning game-making (0374).
- The whole hill in Blender as one mesh: cheapest, but the blend is seen only after export, foliage has
  no layer to grow from, and the view to the horizon is one heavy mesh.
- Shaped in Blender and painted in the editor: the tech-lead's pick. It splits one workflow across two
  tools and still leaves the shape unseen in the engine's picture.
- Keeping 067–080 as they were: they serve the Khronos samples, which 0374 removed as a goal.

## Replaces
nothing. Settles what 0374 left open.
