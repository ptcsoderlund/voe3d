## Assumed
- `voe_editor --check` reports only the first refusal per file, the one `voe_base_report_error_first` keeps.
- The check loads a project's built code as load 0 (`project-0`), so it never clashes with the editor's loads, which count from 1.
- Ranges in `Docs/` are in the file's units (foliage sizes in centimetres as stored), not the Inspector's scaled figures.
- An example project's `Docs/` is ignored by git, because it holds machine-local paths.
- Docs is written under `--capture` too, because capture opens the project like a session.
- A user's `.scene` example placed in `Docs/` is still watched and checked, because the walk does not skip `Docs/`.

## Learned
- The material Inspector's Strength (0–1000) and Repeat (0.01–1000) ranges were literals in `editor/src/inspector_material.h`, not owner constants. 116 moves them to `assets/material.h`.
- A light's intensity has no range row, so the Inspector shows it as a plain number field and `Docs/` gives no range for it.
