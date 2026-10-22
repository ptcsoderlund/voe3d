# 84 — Base has no pool, and a report's state stays on its thread
folder: base
after: 78, 80, 81, 82, 83
decisions: 0168, 0445, 0457, 0459

## Change
Bug 09, 0459 point 4, the last card of the bug: the pool and the report state it carried across
threads go. Each thread's own subjects and kept error (0445) stay as they are.

- `base/include/base/workers.h`, `base/src/workers.c`, `base/tests/workers.c`,
  `base/src/report_state.h` — deleted (`git rm`).
- `base/src/report.c` — `voe_base_report_state_save` and `_load` go, and
  `#include "report_state.h"`. Header: the paragraph on saving and loading the state for the
  pool goes.
- `base/tests/report.c` — the ADR-0448 check (the state carried to a second thread and the empty
  state clearing one), its helpers such as `run_under_loaded_state`, `#include
  "../src/report_state.h"` and `<threads.h>` if nothing else uses it go; the header's ADR-0448
  paragraph goes.
- `base/base.md` — the `include/base/workers.h` entry goes.
- `base/include/base/base.md` — the `workers.h` entry goes.
- `base/src/src.md` — the opening names `report_line.h` alone as internal; the `report.c` entry
  loses "saved and loaded whole by `report_state.h`'s two calls"; the `report_state.h` and
  `workers.c` entries go.
- `base/tests/tests.md` — the `workers.c` entry goes; the `report.c` entry loses "a saved state
  carried to a second thread or cleared by an empty one".

## Done when
`ctest --test-dir build/debug -R '^base/report$'` passes, and
`! grep -rnE "voe_base_workers|base/workers\.h|report_state|voe_platform_cores|models_loader|voe_3d_models_workers"
base platform 3d game/src game/include game/tests editor/src app dev` exits 0.

The human's: open lumberhill in the editor and watch `top`: loading runs on one core.
