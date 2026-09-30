# 0298 — Particles are CPU emitters in 3d, drawn as lit blended quads
date: 2026-09-30
by: planner

## Decision
For 042:
1. **An emitter is a described component in `3d`**, `voe_3d_emitter`, at "Rendering / Particle
   emitter", needing a transform. Fields: playing, rate (a second), burst (fired when it starts
   playing), life (s), speed (m/s), spread (degrees, a cone about `direction`), offset and
   direction (the entity's local space), rise (m/s² along world up), drag (1/s), size start and
   end (m), colour start and end, alpha start and end, glow, texture (a project-relative `.png`
   or `.jpg` path, 128 bytes; empty is the built-in soft dot). Defaults: playing, 20 a second,
   no burst, 1.5 s, 1 m/s, 20 degrees, offset 0, direction +Y, no rise or drag, 0.3 to 0.6 m,
   white to white, alpha 1 to 0, no glow, empty texture.
2. **Its intents** are its replace (the whole row) and a control: play, stop, or burst `count`
   (0 is the row's burst). Stop ends spawning; live particles finish their life.
3. **The particles are a runtime-only row**, `voe_3d_particles`, one per emitter, added and dropped
   by the emitter system as the shape system adds its rows: up to `VOE_3D_EMITTER_PARTICLES` (64) of world
   position (double), velocity, age and life, the spawn debt, a pending burst and a private
   xorshift seed. Particles live in world space: they do not follow the emitter after birth.
4. **`voe_3d_emitter_system_run(world, seconds)`** drains both intents, adds and drops rows,
   spawns, integrates and kills. The game runs it once a fixed step; the editor once a frame
   with the frame's clamped seconds, so an emitter is live while editing.
5. **Pictures live in the model store** (3d/models.h): a `.png`/`.jpg`/`.jpeg` path loads as a
   picture entry, decoded by `assets`, uploaded as one COLOUR texture with two BLENDED parts on
   one shared quad: part 0 lit, part 1 glow (unlit). The soft dot is loaded apart, found at the
   empty path, and not counted. So the loader, the re-read on a changed stamp and the editor's
   failure notice need no new store.
6. **Each live particle is one blended draw** in the world layer, facing the camera, its size,
   colour and alpha from its age through the object's colour. The quad's normals lean outward
   as a sphere's, so a low sun lights one side. Particles cast no shadow.
7. **The blended sort becomes a stable merge sort** with the caller's scratch, since thousands
   of particles are rebuilt unsorted every frame.
8. **Room**: `VOE_GAME_WORLD_EMITTERS` is `VOE_GAME_WORLD_MAX_DRAWN`, because an effect sits on
   a drawn thing and spawned wrecks keep theirs; each emitter's 64 particles are objects in every
   world pass, which is added to the game's and the editor's capacities.

## Reasoning
ADR-0020: work goes to the GPU when it pays. A tank game has hundreds of particles, not
millions; a CPU loop is simpler, can be debugged, and needs no compute pipeline in
`render/vulkan`, which agents do not touch. 0249's GPU direction stays open for a later feature
with a measured need; these particles collide with nothing. Putting pictures in the model store
reuses the path-keyed, stamp-watched loading that the game and the editor already share (0277).
Rejected: a separate picture store (a second loader, a second watcher, and a second parameter
through every pass); additive blending (render has none, and adding it is `render/vulkan`);
instancing (no instanced draw in render).

## Replaces
Nothing. It defers 0249's "GPU compute is for particles" to a feature that measures the need.
