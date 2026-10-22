# 82 — The editor loads models between frames with no pool
folder: editor
after: 77, 79, 80, 81
decisions: 0168, 0370, 0457, 0459

## Change
Bug 09, 0459 points 3 and 4 in the editor: no pool, no loader thread; a load while editing runs
between two frames as before 090 card 60. The update before the pool, for reference only:
`git show e7b5536e^:editor/src/models.c` (later cards changed the rest of that file; keep them).

- `editor/src/models.c` —
  - the struct's `workers` and `loader` fields go; `voe_editor_models_new` makes neither and sets
    nothing on the store; `voe_editor_models_destroy` destroys neither.
  - `voe_editor_models_update` with no progress: `voe_game_models_update`, then
    `voe_game_models_watch` when the once-a-second watch is due, in place of
    `voe_game_models_loader_step`, their failures to the notice as the loader's were; the
    materials call and the rest after as now. With a progress, unchanged.
  - `#include <base/workers.h>`, `<game/models_loader.h>`, `<platform/cores.h>` go.
  - Header: the paragraph on the store carrying the loading pool and the update going through
    `game/models_loader.h` goes; between frames the update and the watch load whole.
- `editor/src/models.h` — header: BETWEEN FRAMES drops "read and baked beside the frames by
  game/models_loader.h (0448 point 5), one put between two frames each frame"; ONCE A SECOND
  drops "The watch goes through the loader too."
- `editor/src/loading.h` — `voe_editor_loading_prepare` loses `workers`; its comment's line and
  the header's "made over the store's pool (0448 point 2)" go; `#include <base/workers.h>` goes.
- `editor/src/loading.c` — the `voe_game_models_prepare` call without `workers` (card 79's
  signature); the comment on the pool handing back errors and LODs made over it goes.
- `editor/src/main.c` — the prepare's `.workers = voe_3d_models_workers(...)` goes.
- `editor/src/src.md` — the `loading.h` entry: "levels made over the store's pool" goes; the
  `models.c` entry: "made with the loading pool it carries" and "beside the frames through
  game/models_loader.h" go.

## Done when
`! grep -rnE "workers|models_loader|cores\.h|beside the frames" editor/src` exits 0, and
`build/debug/editor/voe_editor examples/tank_game --capture "$(mktemp -d)/t.png" 2>&1 | grep -q
"^game: models: "` exits 0.

The human's, on lumberhill (`~/Projekt/voe3dprojects/lumberhill/lumberhill_project`), with
`build/release/editor/voe_editor <project>`:
1. Watch `top` while it opens: loading keeps one core busy, not many; a `game: models:` and a
   `game: materials:` line print each step's time.
2. Drag the tree from Assets into a view: it appears after one longer frame. Import bug 01's
   broken copy: one `error:` line naming it, as before (0445).
