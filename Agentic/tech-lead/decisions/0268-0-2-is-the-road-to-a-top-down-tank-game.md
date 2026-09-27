# 0268 — 0.2 is the road to a top-down tank game
date: 2026-09-27
by: tech-lead

## Decision
The 0.2 game (0267) is a top-down tank game in the spirit of Gunsmoke (NES): a level that scrolls
forward, enemies in waves, water along both sides. The player's tank drives and shoots. Hull,
turret and barrel are separate static meshes that rotate. There are no skinned meshes and no
animation in 0.2. The sponsor makes every model in Blender, with textures baked into channels, and
exports `.glb`. The engine reads `.glb` only, not `.gltf` with separate files. The game is
`examples/tank_game/`, made the way 0251 made the coin game: the sponsor designs its levels, and
agents write its code. It plays on keyboard and mouse and on a gamepad, both supported. Scenery is
destroyed by faking it: a hit swaps in a wreck and plays an explosion. The camera keeps the level's
width in view from above, and a wider window shows more water at the sides. 0.2 is these
milestones, in this order. Each one is one or more features:

1. **Text finished**: the one glyph path (editor and game) draws crisp at small sizes.
2. **The sun in the editor**: the directional light is turned in the editor, and its settings
   (colour, strength and the like) are changed there.
3. **Imported static meshes**: `.glb` models with their baked textures and a material, placed in
   the editor and drawn lit and shadowed like the built-in shapes.
4. **Parenting**: a thing can sit on another and move with it (turret on hull, barrel on turret).
   This needs its own decision amending 0222.
5. **Prefabs and spawning in play**: a saved group of things (an enemy tank) placed in the editor
   and spawned and removed by game code while playing (shells, enemies, wrecks).
6. **The game window**: its size and windowed or fullscreen are set in the project, and the
   aspect is free (ends 0234's fixed 1280×720).
7. **Gamepad**: read on Linux and Windows beside keyboard and mouse, raw like 0239's input.
8. **Hits**: a sweep or ray query for fast shells, beside 0253's overlap.
9. **Particles**: muzzle flash, smoke, explosions, dust from the treads.
10. **Several lights**: short-lived point lights for shots and explosions, lamps in the level.
11. **More sound**: looping sounds, left–right panning by position, a volume per sound.
12. **Water**: a water surface for the sides of the level.
13. **Light bounce**: run-time bounce from the sun, in the shape the ideas list records (a lit
    surface becomes a light, cached, no bake step). Technique chosen by its own decision.
14. **The tank game** built on all of the above, and shipped.

Milestones 1 and 2 come first because the sponsor asked for them first. Anything else goes into
0.2 only when the tank game needs it.

## Reasoning
The sponsor chose a game they will want to build levels for. A tank made of rigid parts forces
the features every real game needs (its own art, parenting, prefabs, spawning, effects, lights),
and it skips skinning and animation, which are a road of their own. Alternatives: a lit first-person
dungeon (it needs a first-person camera and doors, and the sponsor would not enjoy it); a driving
game (physics depth without the art); an arena shooter (a scrolling level makes the level design
matter more).

## Replaces
nothing. Follows 0267.
