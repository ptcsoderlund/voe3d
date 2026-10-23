# 0470 — Cooked copies are one file per model and one per map, made under the splash
date: 2026-10-23
by: planner

## Decision
For 093, how 0454's cooked copy is laid out and when it is made.

1. **A model's copy** is `Cache/models/<path>.model`: the prepared model (level 0, its own or made levels as
   read, the CPU shape, the sphere), the `.glb`'s materials, and every picture of the `.glb` with its full
   mip chain. Each picture is mipped by its use in those materials: base colour and emission as colour,
   normal as normal, metal-roughness and occlusion as data (0466). The file starts with a head: magic,
   version, and the key, which is the platform stamps of the `.glb` and of the `.parts` beside it (0 when
   there is none). The rest is native-endian, sections aligned to 16, and a read points into the file's
   bytes. Any change to the bake or the layout bumps the version. The levels file stays as it is, the
   input Make LODs keeps and remakes (0426, 0429).
2. **A material map's copy** is `Cache/pictures/<path>.<colour|normal|data>`, by the slot that names it:
   colour and emission are colour, normal is normal, ORM is data. Its head is magic, version, the map's
   stamp, kind, size and level count, and the full chain follows.
3. **The mips are made on the CPU.** The full chain runs down to 1×1, each level half the last rounded
   down and never under 1, as Vulkan sizes it. Each level is made from the last by the area average of
   its kind (0466). The levels are stored one after another, so the small ones can be read alone (0454
   point 4). Render takes given levels. A picture with none is still mipped on the GPU, as today.
4. **When copies are made.** A copy is made only where loading waits anyway: an update (open, scene
   load, the game's first load), the materials call, a prepare (Import, Make LODs) and the editor's cook
   before Play and Ship. Watch, reload and a material's own load read a fresh copy but never write one.
   So a file that changes while the editor runs loads from its source, and its copy is made at the next
   open, Play or Ship. Amends 0454 point 3.
5. **The game trusts its copies.** Files copied into a build or an install get new stamps, so the game
   loads any copy whose magic and version match and never compares the key. It makes a copy that is
   missing.
6. **Before Play and Ship** the editor checks every `.glb` under `Assets/` and every material map. It
   reads only the head of each copy (a new platform read of a file's first bytes) and compares the key.
   If anything is stale, it cooks those files under a splash, "Preparing models" done/total. With nothing
   stale there is no splash.
7. **The splash** reads "Preparing models" while a file is being cooked and "Loading models" while one
   is read. Every wait then shows "Loaded in N.N s" for 1 s, measured from the wait's start (not held when
   headless). Amends 0346's "no minimum time" for that line.
8. **Shipping**: `cmake/game.cmake` copies and installs `Cache/models/**` and `Cache/pictures/**`, as it
   does the levels files.
9. Emitter textures, picture entries, landscapes and the splash picture are not cooked.

## Reasoning
- **One file per model, every picture inside it.** A load is then one read and one put, with no lookup
  across files. A map has a file of its own because many materials share one PNG.
- **The key is stamps, not a hash.** A hash means reading the whole `.glb`, which is the cost the copy
  is meant to remove.
- **Cooking only under the splash** keeps frames free of hitches (0386) without a background thread
  (0456). Test step 4 of 093 also asks that only the changed model is prepared at the next open.
- **The game cannot check stamps** on files that CMake copied, so trusting is the only check that works.
  The editor's cook before Play and Ship is what keeps those copies true.

## Replaces
Amends 0454 point 3 (no cooking beside frames; a changed file loads from source until the next open)
and 0346 (the "Loaded in" line is held 1 s). Extends 0426: the made levels ride inside the model's copy.
