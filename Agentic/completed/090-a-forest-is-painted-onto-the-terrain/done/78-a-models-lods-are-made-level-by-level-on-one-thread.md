# 78 — A model's LODs are made level by level on one thread
folder: 3d
after: 77
decisions: 0168, 0436, 0457, 0459

## Change
Bug 09, 0459 point 2: the levels make loses its pool, and with it the arena marks the analyser
flags at `3d/src/model_levels.c:196` (struck card 69). The make before the pool is
`git show 5eee7c1b^:3d/src/model_levels.c`, for reference only: keep today's encoded read
(`voe_assets_model_read_glb_encoded`), own levels and everything else as they are.
`game/src/models_batch.c` still passes a pool here until card 79; `game` does not build between.

- `3d/include/3d/model_levels.h` — `voe_3d_model_levels_make(arena, glb, glb_size, levels, size,
  error)`: the `voe_base_workers *workers` parameter goes, and `#include <base/workers.h>`.
  Its comment: no task, no pool; the made levels are simplified in order on the caller; the
  usage example at the top loses `workers`.
- `3d/src/model_levels.c` — `simplify_levels` loses `workers` and its tasks: for each made level
  in order, an arena mark, each part simplified with `voe_3d_simplify` into a buffer on `arena`,
  the level's triangles summed and held against the level before; one not below it is rewound
  to its mark and ends the list (0436 point 5), else it is kept. No array of marks, no buffer per
  level held before judging, no task arena. `#include <base/workers.h>` goes. Header: the
  "ALL LEVELS ARE SIMPLIFIED AT ONCE OVER THE POOL" paragraph becomes level by level, judged as
  made, one level's buffers given back at most; the opening's "over a pool of workers" goes.
- `3d/tests/model_levels.c` — `pooled_same`, its caller and `#include <base/workers.h>` go; every
  `_make` call loses its `NULL`. Its header's line on the pool goes.
- `3d/tests/models_levels.c`, `3d/tests/draw_levels.c` — every `_make` call loses its `NULL`.
- `3d/src/src.md` — the `model_levels.c` entry: level by level, no pool.
- `3d/tests/tests.md` — the `model_levels.c` entry: "the same over a pool" goes.

## Done when
`ctest --test-dir build/debug -R '^3d/(model_levels|models_levels|draw_levels)$'` passes, and
`3d/src/model_levels.c`'s entry in `build/debug/compile_commands.json`, replayed with
`--analyze -Xanalyzer -analyzer-output=text` as `analyse_entry` in `check/analyser.cmake`
does, prints no `warning:`.
