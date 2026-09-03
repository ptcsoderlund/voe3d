# 018 — model loading: glTF binary

status: todo
claimed-by: -
blocked-by: 017

Written by us (ADR-0023). **Import only — glTF is never a scene format**
(ADR-0010).

## Goal

A `.glb` file on screen, with its own geometry and its own textures — **as
entities in a world**, not as a special case inside `render`. This is the card
that creates the ECS. Read *The world arrives here* below before anything else;
the loading part of this card is the smaller half.

## Scope

- **`.glb` only.** Binary container, no `.gltf` text form. This is a real
  simplification and the reason to take it: no external file resolution, no
  base64 data URIs, no relative-path guessing. Textures come from chunks inside the
  file, decoded by card 017's readers.
- **Geometry**: positions, normals, texture coordinates, indices.
- **Materials: parse them**, per the principal. Base colour, metalness, roughness,
  occlusion, emission, and their texture references. **Stored, not applied** —
  applying them is card 019.
- **No animations.** The principal's decision. Skins, joints and keyframes are
  later, not never.

## Two things that are already decided and must not be re-decided

- **No coordinate conversion, ever.** The engine is right-handed, +Y up, −Z
  forward — *identical to glTF* — precisely so the importer does nothing here.
  ADR-0033 does not merely permit this, it forbids adding one.
- **Matrix layout is transposed, coordinates are not.** ADR-0035, exactly. Getting
  these two confused is the bug that makes a model look mirrored *and* correct
  depending on what you compare it against.

## On material channel packing

The principal's direction is baked channels — **ORM**, and possibly **ORME** with
emission in alpha. Useful fact: **glTF already packs ORM that way** — occlusion in
R, roughness in G, metalness in B of a shared texture is the standard arrangement,
so this is not a divergence. **Emission is where it differs:** glTF's emission is a
full RGB texture, not a single strength channel, so ORME is a real deviation and a
decision that has not been taken. Parse what glTF gives you; do not invent the
packing here.

## The world arrives here — ECS preparation

Every card so far has produced *one* of something: one window, one triangle, one
cube, then two cubes by hand. This card is the first that produces *many* —
meshes, materials, textures, and the objects that use them. Many things need a
place to live, and that place is decided: **the world is an ECS**, flat component
tables and small integer handles, and it lives in the folders `ecs`, `scene` and
`3d`. They are already rows in `cmake/voe.cmake` and in the folder table in
`CLAUDE.md`. They do not exist as directories. This card creates them.

Nothing below is new. It is what `CLAUDE.md`'s rules 2, 3, 4, 10 and 11 and its
*Conventions* section say, applied to the first real call site. Where this text
and `CLAUDE.md` disagree, `CLAUDE.md` wins and this card is wrong.

### Why here and not in `render`

`render/include/render/device.h` says it in capitals: *nothing here accepts a
scene, and the API that will grow one gets designed against `3d`*. This is that
card. If the model were loaded into `render` beside the cube, `render` would
have a scene, and every later card would grow it. So:

- **`render/src/cube.c` is deleted.** Its geometry becomes the first entities;
  its buffers, descriptor layout and upload path become `render`'s public API by
  id (below); its camera and matrices move to `scene` and `3d`. The two cubes
  from card 015 must still be drawable, as two entities in the tables, or the
  offscreen tests have nothing to test.
- **`render` keeps exactly what is Vulkan:** device, memory, buffers, images,
  samplers, descriptors, pipelines, render targets, present, command
  submission. It knows nothing about entities, files, meshes or materials. A gap
  in its API is filled in `render`, by id, with a function `3d` asked for —
  never by `3d` reaching into `device_internal.h`.

### The three folders, and what each owns

| Folder | Depends on | Owns | Does not own |
|---|---|---|---|
| `ecs` | `base` | Entity ids, one table per component type, intent queues and their dispatch by type | Any particular component. `ecs` knows no transform, no mesh |
| `scene` | `ecs`, `math`, `base` | `transform`, `camera` (and `light`, card 019): components and their systems | Anything that names a GPU resource |
| `3d` | `render`, `scene`, `ecs`, `assets`, `math`, `base` | `mesh` and `material` components carrying `render` ids; the system that turns tables into draws; the import step from `assets` data into entities | Vulkan objects. Those stay in `render`, referenced by id |

`assets` (from card 017) parses the file into CPU data and knows nothing about
entities or the GPU. `3d` reads that data, uploads through `render`, gets ids
back, and writes components. That is the whole pipeline: **file → `assets` →
`3d` → ids from `render` → components in tables.**

### `ecs` — entities and tables

- **An entity is a generational id**: an index and a generation, in one 64-bit
  value or two 32-bit halves. Destroying an entity bumps the generation of its
  slot, so a stale id from a destroyed entity is *detected* and fails, rather
  than silently addressing whatever now lives there. Test that specifically.
- **A component type is a table**: one contiguous array of the component
  struct, plus a way from entity id to row and back. Iterating a component type
  is a linear walk over that array with no pointer chasing. That property is the
  point of the ECS and the one thing not to lose; how the entity→row lookup is
  built (sparse set, direct index) is the coder's call, stated in the header.
- **No archetypes, no groups, no hierarchy, no query language.** Flat tables and
  handles were decided over an archetype engine. A system that needs two
  components looks the second one up by entity id. If that ever measures slow,
  it is a later card with a number attached.
- **Capacities are fixed at creation.** The world is created from an arena it is
  handed (rule 11) with a maximum entity count and a maximum per table, given by
  the caller. Running out is a returned failure (rule 13), not a reallocation —
  arenas do not realloc and bounded tables are the deliberate shape.
- **Intents are a queue per intent type, drained by the owning system when it
  runs** (rule 4). Submitting is copying a value into the queue; nothing happens
  until the owning system drains. Submit from anywhere; drain in one place. State
  in the header that an intent submitted during a frame is applied the next time
  its system runs, which may be the same frame or the next depending on order —
  do not promise more than that.

### `scene` — the first components

Two modules, each four files as `CLAUDE.md` names them: `transform_component.h/.c`
and `transform_system.h/.c`, likewise `camera`.

- **`transform`**: position (`float3`), rotation (a quaternion — `math/quat.h`
  exists for this), scale (`float3`). The system produces the world matrix on
  demand or per frame; **no parent, no hierarchy**. A glTF node tree is
  flattened at import (below). A `parent` component is a later card.
- **`camera`**: position and orientation, vertical field of view, near and far.
  **The projection matrix is built in `3d`, not here and not in `math`** —
  `math` is not allowed to know clip space (*Conventions*), and the reverse-Z,
  0..1, negative-viewport-height conventions are `render`/`3d`'s business.
  `scene` holds the numbers a person would author; `3d` turns them into sixteen
  floats.
- **The camera from cards 015/016 moves here.** Whatever drives it — the orbit,
  or card 016's keyboard and mouse — becomes a *submitter of camera intents*.
  Until `app` exists (card 020), the wiring that reads `platform` input and
  submits those intents lives in `dev/src/main.c`, and its header says so. It is
  a call site, not logic: it decides nothing about where the camera is.

Read access is `const` through `*_component.h`; writes happen only inside the
owning module, and from outside only as an intent through `*_system.h`. A file
that includes only `*_component.h` headers is provably a reader.

### `3d` — the components that carry GPU ids, and the draw

This is where the principal's instruction lands: **components tag GPU resources
with an id, and the id is the same number on both sides of the bus.**

- **`render` hands out generational ids** for every buffer and image it creates:
  `voe_render_buffer_id`, `voe_render_texture_id`, or one id type if that is
  simpler — the coder's call, stated. The id is an index into one large
  descriptor array (descriptor indexing, core in Vulkan 1.3, which is already the
  device floor) plus a generation. **A shader indexes textures with the same
  integer the component holds.** No CPU-side map from "component's texture
  number" to "Vulkan's texture number" — the two numbering schemes are one.
- **Geometry lives in a shared pool, and a mesh is a range in it.** `render` owns
  one vertex pool and one index pool, created with the device, capacity a
  parameter, appended to and never freed on this card (say so in the header —
  freeing arrives with the card that unloads something). Uploading a mesh appends
  and returns first vertex, first index and index count.
- **`mesh` component**: that range. Nothing else. It holds no buffer of its own.
- **`material` component**: the factors glTF gives (base colour, metallic,
  roughness, emissive) and **one texture id per texture the material references**.
  A missing texture is a designated *no texture* id that the shader can test,
  not a null pointer. Card 019 reads this component; this card fills it and
  draws with base colour only.
- **Per-object data lives in one GPU buffer, one record per drawn object, per
  frame slot**: world matrix and material index. The shader reads its record by
  object number. **Card 014's push constant for the model matrix goes away.**
  Materials likewise live in one GPU buffer, one record each: factors and
  texture ids. So a shader finds everything about the thing it is drawing by
  three numbers — object, material, texture — and none of them is a Vulkan
  handle.
- **Why ids and ranges, said plainly, so it is not treated as style:** a
  component holding a `VkImage` cannot be written out as text (authoring is text,
  binary at build), leaks Vulkan into every folder that names a texture, and
  cannot be touched by a thread that is not also allowed to touch Vulkan. A
  component holding small integers is plain data: it serialises, it copies, it
  sorts, and any thread may read it. And geometry in one pool with per-object
  records in one buffer is the layout that lets many meshes share one draw call
  later — grass, particles, many objects are a feature of this engine, and this
  is the shape that supports them without a rewrite.
- **The draw, this card:** one system in `3d` walks the `mesh` table in table
  order, looks up each entity's `transform` and `material`, writes the
  per-object records for the frame, and issues **one draw per mesh from the CPU**
  through `render`'s API: begin the frame with the camera's view-projection,
  draw a range with an object number, end. **No sorting, no instancing, no
  indirect command buffer, no culling, no extract or copy step into a second
  layout.** Each of those is a change to this draw loop only, and each is a later
  card with a number attached. What this card guarantees is that the *data* is
  already in the shape they need.
- **Depth makes grouping safe for solid things.** The order and grouping of
  opaque draws does not change the image — the depth buffer resolves visibility
  per pixel. Only blended geometry is order-dependent, and nothing on this card is
  blended. Do not add transparency; it is its own decision.
- **Grouping is the engine's, never the user's.** No component, flag or editor
  concept for hand-grouping objects into batches or buffers. The engine knows the
  grouping key — same pipeline — better than a person does.
- **`render`'s public API grows by exactly what `3d` asks for**, in
  `render/include/render/`, and no more: create a buffer from CPU data and get an
  id; create a texture from decoded pixels and get an id; destroy by id; begin
  the frame's draws into the offscreen target; bind and draw a mesh by ids; end.
  Each function exists because a line in `3d` calls it. `device_internal.h`
  remains internal.

### Import — from `assets` data to entities

- `assets` produces CPU data: meshes as arrays, materials as factors plus image
  references, images as decoded pixels (card 017), and the node tree with
  transforms. It knows nothing else.
- **The node hierarchy is flattened at import.** Each node that references a
  mesh becomes one entity whose `transform` is the node's *world* transform,
  computed by walking the tree once with an explicit stack and a depth limit
  (rule 14). No `parent` component is written. Coordinates are not converted;
  the node matrices are transposed, layout only (*Conventions*).
- Every texture the file's materials reference is uploaded once, and its id is
  written into every `material` component that references it. Two materials
  sharing a texture share the id. That is the deduplication, and it falls out of
  ids being values.
- Import lives in `3d`, because it is the folder that may name `assets`,
  `render`, `scene` and `ecs` at once. It is not a system and writes no
  component it does not own: `transform` values go in through `scene`'s own
  creation call.

### Multithreading and draw-call count — what this card does and does not buy

The principal was told that ids work across CPU and GPU memory and help
multithreading and draw calls. That is right, with the precise version being:

- **Across the bus**: the id in RAM and the index the shader uses in VRAM are
  the same number. That is decided above and built here.
- **Multithreading**: nothing in this card runs on a second thread. What it does
  is remove the obstacles — components are plain data with no Vulkan handle, and
  writes cross folders only as queued intents, which are the seam that later
  lets several threads submit while one system drains. Build no thread pool, no
  job system.
- **Draw calls**: nothing in this card reduces them; one mesh is one draw from
  the CPU. What it does is put the data in the one layout — shared pools,
  per-object records, textures by id — from which a single indirect draw call
  for all opaque geometry is a change to the draw loop and nothing else. That
  switch is a later card, triggered by the first scene with hundreds of objects.

Say so in the folder's `.md`, so nobody later reads the id scheme as a promise
this card kept.

### What this card must not do

- Write a scene *file* format, or the text form of a component. Authoring as text
  is decided; its shape is not, and it is not this card.
- Add a `parent` component, skinning, animation, or more than one camera.
- Put anything that names an entity, a file or a mesh into `render`.
- Put anything that names Vulkan into `ecs`, `scene`, `3d` or `assets`.
- Give a mesh its own buffers. Everything goes through the pools.
- Build for a scene larger than the one loaded. Capacities are parameters.

### Tests this card adds

- `ecs/tests/`: create and destroy entities; a destroyed entity's id is rejected;
  add, get and remove a component; iterate a table and see every live row once;
  submit intents from two places and see the owner drain them in submission
  order; running out of capacity is a returned failure.
- `scene/tests/`: a transform's matrix matches the hand-computed
  translate·rotate·scale for a known input; a camera's numbers round-trip.
- `3d/tests/`: the offscreen back-face and Y-flip tests from card 013 still pass
  with the cube as an entity — they move here or keep working from `render`
  against a `3d`-built scene, coder's call; an import of a tiny hand-built `.glb`
  produces the expected entity count, transforms and shared texture ids.
- `render/tests/`: append two meshes to the pools and get two non-overlapping
  ranges; create a texture, get an id; destroy it; a stale id is rejected; a
  full pool is a returned failure.

### Order of work, suggested

1. `ecs`: world, entities, one dummy component table, intent queue. Tests green.
2. `scene`: `transform`, `camera`. Tests green.
3. `render`: the pools, texture ids, the per-object and material buffers, the
   begin/draw/end API `3d` needs; the shader reads records by number. Existing
   tests green.
4. `3d`: `mesh`, `material`, the draw system. **Move the two cubes into the
   world, delete `render/src/cube.c`, dev shows the same picture as card 015.**
   Stop here and confirm the picture is identical before touching a file format.
5. Import. A `.glb` on screen.

If step 4 alone fills the card, say so and stop there: two cards is a fine
answer and the picture is the proof.

## Verify

- **The two cubes from card 015 are entities, `render/src/cube.c` is gone, and
  the dev window looks exactly as it did.** This is checked before any file is
  loaded.
- Folders `ecs`, `scene` and `3d` exist, each configures standalone, each has
  tests, and `render` names no entity, file, mesh or material.
- No component holds a pointer or a Vulkan handle. `grep -r Vk ecs scene 3d`
  finds nothing.
- A known model, correct orientation, correct handedness, textures on the right
  faces. **A mirrored model is the failure to look for**, and it is invisible on
  anything symmetrical — use an asymmetrical test model.
- Malformed chunk headers, a truncated file and unsupported features are all
  recoverable failures with tests. Unsupported must say *what* it did not support.
- `check.cmake` zero. Windows is the principal's.
