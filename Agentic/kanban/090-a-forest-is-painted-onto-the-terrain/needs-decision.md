# Needs decision — bug 02: how Play gets the models the editor already holds

## The question
Bug 02 expects that "Play uses the models the editor already holds rather than loading them a
second time", and that an unchanged model is not loaded again at editor start. Today Play is a
program of its own (0187, 0235): `cmake/game.cmake` copies every `.glb` (499 MB for the fir tree)
into `Build/debug/`, and the game reads, hashes, parses and bakes it, decodes its six embedded 4k
PNGs and uploads it, exactly as the editor did at start (`game/models.h`, `3d/models.h`). Only the
LOD indices are cached (`Cache/levels/`, 0426). A second process cannot use the editor's memory
without changing what Play is, which reaches every project and every Play, not only this feature.
Which way should model loading go?

## Options
1. **A prepared model in `Cache/`, read by editor and game alike.** Loading a `.glb` writes
   `Cache/models/<path>.model`: the baked parts' vertices and indices, the LOD indices, the welded
   pick shape and the decoded pictures with their mips, keyed by the `.glb`'s stamp (time and size)
   and the engine's format version; nothing hashes the 499 MB unless the stamp moved. Editor start
   reads it and uploads; only a changed file is parsed again, and its LODs made again only when the
   levels file is stale (as today). Play's game reads the project's `Cache/models/` instead of
   copied `.glb`s, so Play no longer copies them; Ship still installs them (or the prepared
   files). Play still uploads to the card a second time, but parses, decodes and bakes nothing.
   Some 6–8 cards across `3d`, `game`, `editor` and `cmake/game.cmake`. Keeps 0187.
2. **Play runs inside the editor's process.** The game's world, steps and window are run by the
   editor beside its own, sharing its model store, device and project library (0242 already loads
   it). The only option that literally never loads twice. Reverses 0187 and 0235's "a program of its
   own": a crashing game takes the editor down, a second window and device sharing, and Play's
   picture and pacing are no longer the shipped program's (0375: "Play shows exactly the shipped
   picture"). A work order of its own, not a bug's cards.
3. **The game process borrows the editor's GPU memory** (Vulkan external memory handed across
   processes). Never parses or uploads twice, but different handle kinds on Linux and Windows,
   both processes on one card, and the editor must outlive and keep still every buffer the game
   uses. Most complex; no other use in the engine.

## Recommendation
Option 1. It answers the slow editor start and the slow Play with one cache, keeps Play the
separate program 0187 and 0375 rely on, and is the same shape as the levels file the engine already
keeps (stale is never an error, `Cache/` may go any time). It reads "loading them a second time" as
"parsing and decoding them a second time"; if the sponsor means no second load at all, that is
option 2 and a work order of its own.
