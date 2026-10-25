# 002 — A model's cook says it is compressing textures
folder: game

## Seen
The editor's loading splash sat on "Loading models 2/3" for minutes with nothing moving. That was `tree asset.glb` being cooked again after 096 made every copy stale. The cook compresses each of the model's pictures to BC7/BC5 on the CPU, and one 8192² picture takes about 4 minutes in debug. The human read it as stuck.

## Expected
While a model is being cooked, and so its textures compressed, the splash says "Compressing textures" with the same done/total count, the way the materials path does (`game/src/models_materials.c:279-281`). The count still moves once per model; a count inside one model is out of scope, since the picture loop is in `3d/`.

## Change
The cook runs inside `voe_game_models_batch_prepare` on the splash's worker thread: `read_task` → `copy_take` finds the copy stale and sets "Preparing models" (`game/src/models_batch.c:339-343`). Then `cook_task` (`game/src/models_batch.c:506-528`) calls `voe_game_models_cooked_save` (`game/src/models_cooked.c:243-248`), which calls `voe_3d_model_cooked_write`, and that compresses every picture. Nothing calls `voe_game_progress_set` from the start of the cook until the next file's put (`game/src/models.c:239-246`). The main thread redraws the splash every frame from the atomic progress record (`game/src/starting.c:256-267`), so a set from the worker shows within a frame.

Set the phase to "Compressing textures", keeping done/total, when a `.glb` is about to be cooked (in `cook_task` before the save, or where `copy_take` sets "Preparing models"). Store it as the file's phase too, so `voe_game_models_batch_phase` reports it. A file whose copy is fresh keeps its current phase. Fix any header or `.md` that names "Preparing models" for this case.

## Done when
A case in `game/tests` prepares a batch holding one `.glb` with a picture and no copy, with a progress record. It checks that the record reads "Compressing textures" after prepare and before put, and that `voe_game_models_batch_phase` for that file says the same. The case passes on a device and does not skip, and `checks.sh --folder game` prints `FINDINGS: 0`.
