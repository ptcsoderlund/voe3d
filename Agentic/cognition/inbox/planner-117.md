## Assumed
- The species file's extension is `.tree`, and its angle is in degrees, as landscape looks' turn is.
- A `crown` setting (share of height where branches start) is added beyond the feature's list.
- Variations are capped at 16, the same as a landscape's kinds, to fit the store's 128 entries.
- A species deleted from Assets keeps drawing its last growth until the project is opened again.
- The species view's Save writes no world; the set-aside level keeps its unsaved flag until Back.
## Learned
- A foliage copy stores x and z as integer millimetres, so a hash of them is stable across save and reopen.
- `voe_3d_models_parts_set` with no row empties a part's `wears`, so grown entries must be exempt or lose their bark and leaf.
- The game's watch keeps a loaded entry whose file is gone as failed at 0, so any entry with no file behind it must be skipped by the watch.
- `editor/src/models.c` (856) and `game/src/models_batch.c` (909) are over 800 lines; `project.c` is at 799.
