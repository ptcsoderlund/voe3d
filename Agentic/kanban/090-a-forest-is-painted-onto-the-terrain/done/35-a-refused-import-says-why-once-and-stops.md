# 35 — A refused import says why once, and stops
folder: editor
after: 34
decisions: 0168, 0443, 0445

## Change
0445 points 1, 4 and 6 for Import. Today a `.glb` the reader refuses is read again by the LODs
question, the prepare and the reload, each saying the same cause once more.

- `editor/src/notice.h`, `editor/src/notice.c` — `voe_editor_notice_from_report`: the kept line
  alone when it already begins with `<file>: ` (a subject named it), else `<file>: <kept>` as
  today; the fallback stays. Header: why the file is not named twice (0445 point 6).
- `editor/src/model_import.c` — `glb` pushed as the subject (card 23's
  `voe_base_report_subject_push`) around `model_read`'s read and parse, and around Import
  textures' read; popped on every path. The "more materials than a parts file names" refusal is a
  limit in 0443's words (how many the model uses, the most, join materials in Blender), reported
  once through `VOE_BASE_ERROR("editor", …)` under the subject, and the notice built from it.
- `editor/src/assets_panel.c` — in `voe_editor_assets_import`, a `.glb` that
  `voe_editor_model_import` refuses leaves `imported` empty, so `session.c`'s `session_import`
  opens no LODs question and nothing prepares or reloads it; the `.glb` stays copied and listed.
  The comment above the call says so.
- `editor/src/assets_panel.h` — `voe_editor_assets_import`'s comment: `imported` is "" also when
  the import refused the `.glb`.
- `editor/src/src.md` — the `notice.c` and `model_import.c` entries.

## Done when
`! grep -q "(void)voe_editor_model_import(" editor/src/assets_panel.c` exits 0 and
`grep -q "voe_base_report_subject_push" editor/src/model_import.c` exits 0.
