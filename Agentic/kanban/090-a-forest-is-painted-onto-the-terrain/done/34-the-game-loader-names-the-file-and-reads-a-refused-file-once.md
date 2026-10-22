# 34 — The game loader names the file and reads a refused file once
folder: game
after: 33
decisions: 0168, 0443, 0445

## Change
0445 points 1, 3 and 4 in the loader the game and the editor share. `base/report.h`'s subject
push and pop (card 23) is how the file gets into every line below it.

- `game/src/models.c` —
  - the project-relative `path` pushed as the subject around each file's read, levels make or
    read, and store load: in update and watch, in `voe_game_models_prepare`, `_reload` and
    `_levels_triangles`; popped on every path out.
  - "could not read the model %s: %s" goes where the store's load refused (its cause has a line);
    a file that cannot be stamped or read keeps one line under the subject saying the file could
    not be read and the category, no path in the message.
  - "could not write the levels of %s": under the subject, the LODs could not be saved in the
    project's `Cache/levels/`, and the category.
  - one pass per cause: in update, watch and reload, when making the levels file refuses the
    `.glb`, the entry is kept failed at the file's stamp through `voe_3d_models_fail` and the
    store does not read the file a second time; counted as a failure as today.
  - header: the subject, the one line per cause, the refused make ending the passes.
- `game/src/models_materials.c` — the material's path pushed as the subject around its maps' reads
  and its load; "could not load the material %s: %s" goes where a cause was reported below.
- `game/include/game/models.h` — comments only: BOTH RETURN's "each failure is one line on stderr
  naming the path and the category" becomes one line at the cause, under the path as subject
  (0445); LEVELS says a make refusing the `.glb` keeps it failed with no second read.
- `game/src/src.md` — the `models.c` and `models_materials.c` entries.
- `game/tests/models_levels.c` — new check: a `.glb` of bad bytes in the test's folder, named by a
  row: after an update its entry is failed at the file's stamp, no levels file is in
  `Cache/levels/`, and the kept first error (cleared before) begins with its project-relative
  path. Existing wording checks updated.
- `game/tests/tests.md` — the `models_levels.c` entry names the check.

## Done when
`ctest --test-dir build/debug -R '^game/(models|models_levels)$'` passes with the new check, and
`! grep -q "could not read the model %s: %s" game/src/models.c` exits 0.
