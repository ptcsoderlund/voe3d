# 006 — The model watch starts no work for an unchanged file
folder: game

## Seen
The editor's once-a-second model watch starts and joins a burst of threads for every model even when its
file is unchanged. Under CLion's debugger on Windows each thread start and exit pauses the process, so
the maximized editor stutters for seconds with CPU and GPU idle. Most of the time lands in the hitch log's
`step` span:

```
editor: hitch: frame 2156.3 ms (mean 105.2): step 1872.8, views 143.1, shadows 87.4, pace 72.3, interface 29.5, light bounces
```

## Expected
The watch compares each model file's stamp first and starts no work for an unchanged file. An unchanged
project costs the watch a stat per model each second and no threads at all. A changed file still
reloads as now.

## Change
`voe_game_models_watch` (`game/src/models.c:616-623`) builds a one-file batch for every model in the store
except landscapes and materials (`watch_gather`, `game/src/models.c:595-597`), whether or not the file
changed. Each batch then runs `voe_game_models_batch_prepare` (`game/src/models_batch.c:714-726`), whose
phases (read, parse, LODs, bake) each start and join one burst (`platform/src/burst.c:76-79, 95-96`). The
stamp is only compared later, inside `read_task` (`game/src/models_batch.c:410`), on a worker.

Move the stamp comparison ahead of the gather. The watch puts a model in a batch only when the same stamp
test `read_task` makes says the file changed, and an empty gather starts no batch and no burst. Keep one
stamp test that both places use, so the watch and the reload cannot disagree about what "changed" means.
The caller in the editor (`editor/src/models.c:258-265`) stays as it is.

## Done when
- A game test: a store with loaded models whose files are unchanged runs the watch and gathers nothing,
  so no batch is prepared. Touch one file's stamp and the watch gathers that one model alone, and the
  reload happens as before.
- The game folder's check passes.
- On Windows under CLion's debugger, the maximized editor with the hill open no longer shows thread
  starts and exits once a second in the debug console while it sits still (checked by the human).
