# 09 — The editor cooks and queues on its device
folder: editor
after: 04, 05, 06, 07, 08
decisions: 0168, 0370, 0484, 0486, 0487, 0489
read: feature.md

## Change
Cards 04 and 07 gave game's prepare, cook and map queue a device; this card moves the editor's
calls onto them and makes the feature's How to test true.

- `editor/src/loading.h` — the prepare record (near line 89) and the cook record (near line 102)
  gain `voe_render_device *device`, as the start's and load's records have; the header's lines
  that say prepare and cook run with no device (near lines 19 and 24) say instead that they
  compress on the program's own device under its guard (0486), as the worker already may (0370).
- `editor/src/loading.c` — `voe_game_models_prepare` and `voe_game_models_cook` are passed the
  record's device.
- `editor/src/waits.c` — the import's and Make levels' prepare record and the cook record are
  filled with `waits->device`.
- `editor/src/models.c` — the queue's `_clear` (in `follow_folder`) and `_destroy` pass the device
  the function holds; `_step` is called once an update with the device and scratch, its path
  reloading the materials naming it as now; `COMPRESS_SECONDS` and its comment go; the file's
  top line about the queue says it compresses on the GPU.
- `editor/src/models.h` — COMPRESSING BETWEEN FRAMES rewritten per 0489: outside a wait a map with
  no fresh copy is queued and its material loads with that map empty; a step never waits (burst,
  GPU, burst); when the copy is written every held material naming it loads again. Under a
  splash maps are compressed as progress. The 2 ms, the CPU encoder and "draws uncompressed" go.
- `editor/src/src.md` — `models.h`'s entry: maps compressed on the GPU between frames, not 2 ms a
  frame; `loading.h`'s: prepare and cook on the device.

## Done when
The editor builds, and
`grep -rn "COMPRESS_SECONDS\|no device" editor/src/models.c editor/src/loading.h` prints nothing.

The human's, in the debug build normally run:
1. Close the editor, delete the project's Cache folder, open the project: the splash shows its
   steps and the project is ready in under 30 seconds.
2. Look at models, ground and materials near and far: no blocky patches, no colour bands, normal
   maps as smooth as before.
3. Frame panel: every material texture counted as compressed, texture memory about as before.
4. Close and open again: nothing is compressed.
5. Inspector: change a material's texture to another picture; within about a second it shows,
   compressed, and the view does not stutter.
6. Import a model with 4096-side textures and place it: ready within a few seconds.
7. Delete the Cache folder, press Play: it starts within a few seconds of the usual time. Ship,
   delete the shipped game's cache if it has one, run it: no long wait, looks as in the editor.
