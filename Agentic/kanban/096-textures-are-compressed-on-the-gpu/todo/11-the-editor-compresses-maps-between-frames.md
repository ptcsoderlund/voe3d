# 11 — The editor compresses maps between frames
folder: editor
after: 10
decisions: 0168, 0386, 0388, 0470, 0477

## Change
The editor's side of 0477 point 3, and the callers card 10 broke. A map with no fresh copy draws
uncompressed at once. It is compressed at most 2 ms a frame, between frames, and its materials then
load again from the written copy. This is the frame's new part, named by 0388.

- `editor/src/models.h`, `editor/src/models.c`:
  - the store owns a `voe_game_compressing`: made in `_new`, cleared when the folder changes,
    destroyed in `_destroy`;
  - `_update` passes its `progress` to `voe_game_models_materials`, and the queue only when
    `progress` is NULL. Under a splash, maps are compressed inline and show as progress;
  - `material_reload` passes the queue to `voe_game_models_material_load`;
  - outside a wait, after the materials: `voe_game_compressing_step(queue, 0.002, scratch)`. A
    returned path queues for reload every table row naming it in any of its four map slots, through
    the existing `reload` list and a small helper. The next update loads those rows from the copy.
  - The header gets a paragraph on compressing between frames: 2 ms a frame, a map drawn
    uncompressed until its copy is written, why not under the frame (0477 points 3 and 4).
- `editor/src/src.md` — the `models.h` and `models.c` entries.

## Done when
The editor builds, and `grep -n "voe_game_compressing_step" editor/src/models.c` prints a line.
