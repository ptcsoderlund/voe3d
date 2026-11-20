# 11 — Undo carries file and landscape steps
folder: editor
after: 10
decisions: 0168, 0204, 0506, 0509

## Change
- `editor/src/undo.h`, `editor/src/undo.c`: a state may carry a `voe_editor_file_step *file` or a
  `voe_editor_landscape_step *landscape` beside its text, the way it carries a material step.
  - `voe_editor_undo_file(undo, project, scratch, step)` and
    `voe_editor_undo_landscape(undo, project, scratch, step)` each push a state whose text equals
    the one before, as `voe_editor_undo_material` does. The undo line owns the step.
  - `voe_editor_undo_take` applies either step back or forward with its `_apply`, using the watch
    it already takes, and says a refusal in `why`.
  - Every path that drops a state frees its step: `_forget`, the line cut at 64, and a new edit
    past the top.
- undo.h's header: a step may be a file taken in from disk (0506), and what back and forward do for
  each kind.

## Done when
- `grep -q voe_editor_undo_file editor/src/undo.h` and
  `grep -q voe_editor_undo_landscape editor/src/undo.h` both exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
