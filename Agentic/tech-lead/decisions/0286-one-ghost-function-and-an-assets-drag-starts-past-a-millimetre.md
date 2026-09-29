# 0286 — One ghost function, and an Assets drag starts past a millimetre
date: 2026-09-29
by: planner

## Decision
For 038 bug 02, how 0285 is carried out in the editor:

1. **One ghost function** in `editor/src/drag_ghost.h` draws every drag's ghost: a raised panel
   of a name beside the pointer (0282 point 2); when refused, drawn under the Scene list's dim
   theme (0282 point 3, `scene.list_dim`) with a second line "Can't drop here".
2. **An Assets panel drag shows its ghost only once the pointer has moved
   `VOE_EDITOR_SCENE_DRAG_START` from the press**, as a Scene list drag does, so a click that
   opens a prefab never flashes one. A release before that drops nothing, as before.
3. **The Assets ghost names the file**: the path's last segment, extension kept, as its row
   shows it.
4. **Refused is worked out after the frame and drawn the next**, as 0282 point 5, by the one
   function the release uses: for an Assets drag the outcome at the pointer in `assets_drag.c`;
   for a Scene list drag no row or heading target and not over an Assets panel that takes it.
5. **Over the Assets panel a Scene list drag is taken when the make would be**, by a new
   `voe_editor_prefab_make_refused` holding every refusal of `voe_editor_prefab_make` except
   the file already existing (a file test every frame is not worth it); the make calls it first.
   A refused release there still reaches the make, whose notice says why.

## Reasoning
One function keeps every later drag's ghost the same (0285). The threshold is 0282's reason
again: a click is not a drag. Reusing `list_dim` needs no second pushed theme. Rejected: a
ghost per panel (drifts apart); testing the file's existence each frame (disk I/O per frame for
a rare case the make reports anyway).

## Replaces
nothing. Carries out 0285; reads 0282, 0283.
