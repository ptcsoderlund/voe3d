# 0368 — 0.3 is the Amazon Bistro, flown through from day to night
date: 2026-10-05
by: tech-lead

## Decision
0.3 is chosen by a scene, not a game: Amazon Lumberyard's Bistro, exterior and interior, drawn well
enough to stand beside other engines' renders of it. It is a fly-through with no gameplay, both in the
editor and as a shipped build. In the build you fly with keyboard and mouse as in an editor view, and a
time of day moves the sun and moon from day to night. The scene comes in as `.glb` files the sponsor
exports from Blender (0268). It is not committed to git, for its size and its licence. Done means 60 fps at
2560×1440 on the sponsor's own machine, by 062's frame breakdown, anywhere in the scene, at any time of day.
The materials are the ones the scene ships with, read by the importer: base colour, normal, roughness and
metal, emission, alpha-cut, two-sided and glass. 0.3 has no custom shaders and no material editor. Skinning
and animation wait for 0.4. 0.3 is these milestones, in this order. Each one is one or more features:

1. **The Bistro loads and draws**: every object and material of both parts, plants cut by their alpha
   and drawn from both sides, glass seen through, emission, compressed textures, and only what the camera
   sees drawn.
2. **Exposure, tone and bloom**: a picture readable in full sun and at night without touching a
   setting, and bright lamps that glow.
3. **Sky and time of day**: a drawn sky lit by the sun, which also lights the scene. A time of day sets the
   sun and the moon. The editor changes it, and the fly-through has a key to run it.
4. **Night lighting**: the street lamps and lit windows as many small lights at once. Inside, light-blocking
   volumes (056) keep the sun and sky out, and the bounce (051) works indoors.
5. **Volumetric fog**: fog lit by the sun and the lamps, with the sun's shadows in it as god rays.
6. **Smooth edges and contact shadows**: no shimmer on plants, railings and wires while moving, and ambient
   occlusion in corners and under things.
7. **60 fps at 1440p**: whatever 062's breakdown names as the cost, fixed. LOD only if geometry is that cost
   (the run-time LOD line in the ideas list).
8. **The fly-through shipped**.

Texture streaming is not in 0.3: compressed, the Bistro fits in video memory. Anything else goes into 0.3
only when the Bistro needs it.

## Reasoning
The Bistro is the scene renderers are compared on. It forces what 0.2 skipped: scale, a sky, exposure,
fog, many lights, anti-aliasing. 062 lets each step be measured. The interior is in so the light-blocking
volumes and indoor bounce get a real test. A fly-through and not a game keeps the release about how
things look. The scene's own materials are standard PBR, so custom shaders can wait for a game that needs
a look the standard ones cannot give.
- A game with Bistro as its level: gameplay work that dilutes the release.
- Exterior only: easier, but leaves 056 and indoor bounce untested.
- Skinning and animation now: a road of their own, and the Bistro has none.
- Texture streaming and LOD streaming: for open worlds that do not fit in memory. Kept in the ideas list.

## Replaces
nothing. Amends 0267: 0.3 is chosen by a scene, not a game.
