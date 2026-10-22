# 81 — The model store holds no pool
folder: 3d
after: 78, 80
decisions: 0168, 0457, 0459

## Change
Bug 09, 0459 point 4 in 3d: the store stops carrying a pool, and nothing here says a load runs
on another thread. The prepare and put split, `voe_3d_models_put_material` and the picture
decode stay: they are where the loader's steps are timed (0459 point 1). The editor still calls
the two pool calls until card 82; it does not build between.

- `3d/include/3d/models.h` — `voe_3d_models_workers_set`, `voe_3d_models_workers`, their comment
  and `#include <base/workers.h>` go; the header's "THE STORE CARRIES THE POOL" sentence goes;
  `voe_3d_models_clear`'s "The pool is kept." goes; `voe_3d_models_put_material`'s "decoded
  already on any thread" becomes decoded by the caller before the put.
- `3d/src/models.c` — the two calls and the "THE POOL IS HELD, NOT OWNED" paragraph go; the
  header's "on any thread" for the prepare goes.
- `3d/src/models_store.h` — the `workers` field and `#include <base/workers.h>` go.
- `3d/include/3d/model_prepare.h` — header: the prepare is the baking step and the put the
  uploading step of the game's timed load (0459), both on the caller's thread; "on any thread",
  "side by side" and "decoded … on other threads" go. What the prepare touches (no device, no
  store) and the model kept by pointer, its pictures decoded before the put, stay.
- `3d/src/models_material.c` — header: "a loader decoded already on other threads" becomes the
  loader's decoding step.
- `3d/tests/models.c` — `check_workers` and its call go. Its header's line on the pool, if any.
- `3d/tests/models_levels.c` — the prepare on a second thread (`prepare_task`, `thrd_create`,
  `thrd_join`) runs on the test's thread instead, the same checks after; the header's case says
  prepared, decoded and put on one thread. `<threads.h>` goes if nothing else uses it.
- `3d/src/src.md` — `models.c` entry: "the pool held, not owned" goes; `models_store.h` entry:
  "with the program's pool" goes.
- `3d/include/3d/3d.md` — `models.h` entry: "and the pool its loaders run on" goes;
  `model_prepare.h` entry: "on any thread" goes.
- `3d/tests/tests.md` — `models.c` entry: "and the pool" goes; `models_levels.c` entry: "a
  prepare on a second thread" becomes a prepare then a put.

## Done when
`ctest --test-dir build/debug -R '^3d/(models|models_levels|models_material|draw_levels)$'`
passes, and `! grep -rnE "voe_base_workers|workers_set|thrd_create|on any thread|other threads"
3d/src 3d/include 3d/tests` exits 0.
