# 0449 — Models and pictures are cooked raw into Cache/, and every load reads the cooked copy
date: 2026-10-20
by: tech-lead

## Decision
1. **One cooked format.** A model's meshes, LODs and pictures (with their mips) are cooked into the project's
   `Cache/`, laid out as they sit in memory, so a load is read and upload with no decoding or rebuilding. The
   editor, Play and the shipped game all read the cooked copies, so Play no longer loads from scratch. The
   `.glb` and PNGs in `Assets/` are what gets cooked. A cooked copy is made again when its inputs change, and
   deleting `Cache/` only costs a re-cook. Pictures are cooked as RGBA8 until 094 cooks them GPU-compressed.
2. **Raw, not squeezed or packed.** Cooked files ship as they are: no general-purpose compression and no pack
   file. Both wait until the shipped hill's size has been measured (096). If it is too big, a fast lossless
   squeeze and a pack file are their own work order.
3. **Cooking runs on one thread (0451).** It happens under the splash, or beside frames without hitching
   them (0386) when a model changes while the editor runs.
4. **Mips stay separable.** A cooked picture keeps its mips so that the small ones can be read without the
   large ones, for texture streaming later (ideas.md).

## Reasoning
A cooked copy means the work is done once, and every
later open, scene load and Play only reads. `.glb` stays the import format (0403). It already stores meshes
raw; the cost is in decoding pictures, building mips and making LODs. Raw RGBA8 is about five times a PNG's
size, but GPU-compressed (094) it is about the PNG's size and needs no decoding, so squeezing earns little
until measured. Alternatives:
- Loading on every core (0447). Rejected: every load still repeats all the work; withdrawn by 0451.
- Replacing `.glb` in `Assets/` with our own format. Rejected: Import and re-export from Blender stay simple,
  and the cooked copy is disposable.
- Compressing and packing now. Rejected: no measurement says it is needed (0358).

## Replaces
Replaces 0447's "no prepared-model cache now, and Play loading again is accepted" (0447 itself is withdrawn
by 0451). Extends 0426: the levels
file becomes part of the cooked copy, and how they combine is the planner's.
