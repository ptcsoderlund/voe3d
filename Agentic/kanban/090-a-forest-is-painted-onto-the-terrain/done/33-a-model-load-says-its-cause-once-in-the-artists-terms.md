# 33 — A model load says its cause once, in the artist's terms
folder: 3d
after: 32
decisions: 0168, 0443, 0445

## Change
0445 points 2, 3 and 5 in `3d`. The file is the report's subject, pushed by the code that opened it
(`game`, `editor`; card 23's `base/report.h`), so nothing here names a path in a message any more.

- `3d/src/models.c` — the lines restating a reported cause go: "could not load the model %s",
  "could not load the landscape %s". Where a load fails at a step that printed nothing, the load
  reports that cause itself, once. A full store is a limit: how many files the store holds at
  once (`VOE_3D_MODELS`), the most the engine takes; no path in it. The soft dot's and the water's
  lines stay (they are built in, not imported). Header: a failed load says its cause once, at the
  step that met it (0445).
- `3d/src/model_bake.c` — its three refusals as limits, with the file's number beside the most:
  materials past `VOE_3D_MODEL_PARTS` (how many the model uses; join materials in Blender, or
  split the model into files); nodes nesting past their depth (how deep; Clear Parent and Keep
  Transform); vertices or indices past a 32-bit count (the model's total, the most, Decimate).
  Count the actual number before reporting it.
- `3d/src/models_material.c` — "the map %s is no picture": which map (colour, normal, ORM,
  emission) and its path, not a PNG or JPEG picture, save it as PNG; each map decoded with "the
  <kind> map `path`" pushed as a subject, so a decoder's line names it; "could not load the
  material %s" goes where a cause was reported below.
- `3d/src/import.c` — its "no room" line in the same terms, as the limit or the card's memory it
  is, and its nesting line as `model_bake.c`'s.
- `3d/include/3d/models.h` — `voe_3d_models_load`'s comment: every failure is one line at the step
  that met it, in the artist's words, under the caller's subject; nothing here names the path.
- `3d/src/src.md` — entries of these files where they name messages.
- `3d/tests/models.c` — new checks on `voe_base_report_error_first()` after a clear, with
  `Assets/many.glb` pushed as the subject around the load: a `.glb` with one more material than
  `VOE_3D_MODEL_PARTS` is refused and its kept line begins "Assets/many.glb: " and names both
  counts. Existing wording checks updated.
- `3d/tests/tests.md` — the `models.c` entry names the check.

## Done when
`ctest --test-dir build/debug -R '^3d/(models|models_material|import)$'` passes with the new
check, and `! grep -q "could not load the model" 3d/src/models.c` exits 0.
