# 06 — Errors and Project close by an × in a title row
folder: editor
after: 05
decisions: 0168, 0363

## Change
0363 point 2 for the two panels over the dock. How each is shown and hidden does not change.

- `editor/src/errors.h` and `errors.c`: the "Build errors" title shares a row with an × (U+00D7) button at
  its right; Close's row goes; `close_button` records the ×, so `voe_editor_errors_clicks_read` answers for
  it. Anything else on the panel (its lines, Copy all) stays.
- `editor/src/project_panel.h` and `project_panel.c`: a title row "Project" with the same × at its right,
  first on the panel; Close's row goes; the result's `closed` fires on the ×.
- `editor/src/interface.h`: the Errors and Project panel budget paragraphs recounted for the title rows in
  Close's place, and the three numbers changed if the totals do.
- Both headers' examples and descriptions say × where they said Close. `editor/src/src.md`: the `errors.h`
  and `project_panel.h` entries say ×.

## Done when
- The folder builds.
- `! grep -q '"Close"' editor/src/errors.c editor/src/project_panel.c` exits 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
