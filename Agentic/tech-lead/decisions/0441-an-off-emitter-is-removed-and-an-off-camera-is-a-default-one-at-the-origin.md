# 0441 — An off emitter is removed, and an off camera is a default one at the origin
date: 2026-10-16
by: planner

## Decision
For 089 bug 11, carrying out 0427 for emitters and the camera.
1. An emitter whose entity is off (0428 point 2's read) has no particles row: the emitter system drops
   it at the next run and adds none while it is off, so nothing is spawned, moved or drawn. Its
   controls still set and clear `playing`, the component's own data, as an off sound keeps `playing`
   (0428 point 6); a burst asked while off is lost. Switched on, it gets a new row as a newly added
   emitter does: seeded from the entity, taking the row's burst when it plays. Nothing is kept warm;
   a warm start would be the emitter's own setting, not the switch's.
2. The camera's entity has the Enabled switch like any other; Duplicate and Delete stay refused
   (ADR-0218). The table still holds exactly one camera.
3. A camera that is off, itself or through a parent, frames as a camera at the identity transform
   (the origin, unturned, unit scale, looking down -Z) with the camera table's default row, in
   `voe_3d_draw_system_frame`; so the game's frame, Play, Ship, dev and the editor's corner picture
   follow from that one place.
4. Picks and the editor's camera marker pass an off camera by, as any off entity (0428 point 7).
5. The sound listener of an off camera stands where 3 puts the view: at the origin, +X right, -Z
   forward, the default row's field of view.

## Reasoning
The human's words: the off switch makes a thing act as if deleted, and keeping particles warm is a
feature of the component. Dropping the row reuses the path a removed emitter already takes, so on
again is exactly a new emitter. A world must still be drawn from somewhere, and the identity pose
with the default lens is what "no camera" means without inventing a fallback. Hearing from where one
sees keeps sound placed as the picture is.

## Replaces
0428 point 8; the last sentence of 0428 point 3 ("its births go on").
