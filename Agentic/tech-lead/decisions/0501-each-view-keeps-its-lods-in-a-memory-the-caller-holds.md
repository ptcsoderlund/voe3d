# 0501 — Each view keeps its LODs in a memory the caller holds
date: 2026-10-30
by: planner

## Decision
Settles how 0500 point 1's per-view memory is built, for work order 106:

1. **One memory per drawing target, held by the caller.** `voe_3d_lods_kept` in 3d, made once on the
   caller's arena: one per editor view, one for the preview, one for the game's window (Play and a
   shipped build alike). The caller begins it each frame and sets it in the frame, as the bounce
   casters are (0394 point 4). None in the frame chooses with no state and draws one LOD whole: dev,
   tests and the bounce's captures.
2. **The dissolve runs on the wall clock.** Begin takes seconds of the platform's steady clock
   (`voe_platform_clock_now`) every frame drawn. A paused world or a paused Play does not stop a
   dissolve; 0.25 s of wall time ends it.
3. **First sight is "not chosen in this memory's previous frame".** A copy culled last frame, a view
   not drawn last frame, a new view: each takes its distance's LOD whole. Choosing a copy twice in one
   frame (camera pass and shadows) gives the same answer.
4. **A copy's identity is its place.** A model row is its entity; a foliage copy is a 64-bit mix of
   its landscape row's entity, its kind index and its painted x and z, the two kinds of key told apart
   by the top bit. A copy a repaint or a regrow leaves in place keeps its memory; a moved one is new.
   Two keys that collide share one memory, which can only show one LOD early.
5. **Which copies remember.** Model rows, and foliage copies of the seen and cast tiers. Unseen-tier
   foliage chooses with no state. Shadows and held parts draw the kept LOD whole.
6. **Room.** `VOE_3D_LODS_KEPT` copies a frame, the foliage instance cap (2^17); two tables, this
   frame's and the last, told valid by a frame stamp so nothing is cleared each frame. A copy past
   the room chooses with no state and draws whole.
7. **The default lens is a named constant.** `VOE_SCENE_CAMERA_FOV_Y`, the 60° a new camera gets, is
   what the panel's Swap distance is measured through.

## Reasoning
The memory has to live as long as a view, which `voe_3d_frame`, built each frame, does not; the
casters already set this pattern. A wall clock is the only one every target has, and a dissolve that
stalled with a paused world would leave copies stippled, against 0500 point 1. "Chosen last frame" is
one stamp compare and covers culling, new views and undrawn views with one rule. Painted copies have
no entity; their place and kind are what the landscape file keeps, so a key from them survives saves,
repaints and regrowth. Alternatives: a copy index (renumbered by every repaint), a world-time clock
(stops in a paused editor), a memory inside 3d per view index (3d does not know the editor's views).

## Replaces
Nothing.
