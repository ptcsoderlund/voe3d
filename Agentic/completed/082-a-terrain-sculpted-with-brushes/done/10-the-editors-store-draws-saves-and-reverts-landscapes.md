# 10 — The editor's store draws, saves and reverts landscapes
folder: editor
after: 03, 09
decisions: 0168, 0379

## Change
The editor's model store makes a frame's transient chunks, settles them, writes edited landscapes on
Save and reads them again on a New or Open (0379 points 4 and 6). Nothing edits one yet; card 13 does.

- `editor/src/models.h` / `models.c` — new, each a thin call into 3d/models.h that only this file makes:
  - `voe_editor_models_frame(models, gpu, scratch)` — `voe_3d_models_landscape_frame`.
  - `voe_editor_models_settle(models, gpu, scratch)` — `voe_3d_models_landscape_settle`, a failure one
    stderr line.
  - `[[nodiscard]] bool voe_editor_models_save(models, const char *folder, scratch,
    voe_editor_notice *why)` — every entry with a landscape and `edited`: written with
    `voe_assets_landscape_write` to `<folder>/<path>` through `platform`'s file write, then
    `voe_3d_models_landscape_saved`; false with `why` naming the file at the first refused write.
  - `voe_editor_models_revert(models, const char *folder, gpu, scratch)` — every edited landscape read
    from its file again and loaded over itself (`voe_3d_models_load`); a failure as the update's.
  - Header points: who edits a landscape and why only through this file; transient and settle per frame.
- `editor/src/session.h` / `session.c` — `bool saved`, set by a SAVE (and the browser's Save confirm)
  that wrote the project, cleared by main.c, documented beside `replaced`.
- `editor/src/main.c` — after the frame's draw opens and before `voe_editor_view_passes_preview`:
  `voe_editor_models_frame`; beside `voe_editor_models_update`: `voe_editor_models_settle`; where
  `session.saved` is seen: `voe_editor_models_save`, a refusal put in the session notice and the project
  marked unsaved again; where `session.replaced` is acted on: `voe_editor_models_revert`.
- `editor/src/view_passes.h` — `VOE_EDITOR_CAPACITIES`' transient vertices, indices and geometries add
  `VOE_3D_LANDSCAPE_TRANSIENT_*` (3d/models.h); the paragraph above it says why.
- `editor/src/src.md` — the `models`, `session` and `main.c` entries take the new parts in a phrase.

## Done when
`grep -c voe_editor_models_frame editor/src/main.c` prints 1 or more, and the folder's check passes.
