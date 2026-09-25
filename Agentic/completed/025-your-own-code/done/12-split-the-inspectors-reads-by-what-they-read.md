# 12 — Split the Inspector's reads by what they read
folder: editor
decisions: 0168

## Change
`editor/src/inspector_edit.c` is 777 lines and card 13 adds an entity list to its dropdown read.
Split it by function first; behaviour unchanged. Read `editor/src/inspector_edit.h`'s header and
the function list of inspector_edit.c (its top-level definitions, not their bodies).

- `editor/src/inspector_edit.c` keeps the edits: the static `submit`, `apply`, `typed`,
  `voe_editor_inspector_edits_read`, `_colour_submit`, `_named_submit`.
- `editor/src/inspector_place.h`, `editor/src/inspector_place.c` (new) — where an overlay goes:
  `contains`, `side_and_cap`, `voe_editor_inspector_overlay_place` (its declaration and the
  side-and-cap part of inspector_edit.h's header move here) and `submenu_place` (made non-static
  and declared here if the buttons file calls it).
- `editor/src/inspector_buttons.c` (new) — what a button fired: `action_of`, `counted`, `row_of`,
  `group_open`, `voe_editor_inspector_add_close`, `voe_editor_inspector_buttons_read`, still
  declared in inspector_edit.h. A helper both new files need is declared in inspector_place.h,
  never duplicated.
- Every file that calls `voe_editor_inspector_overlay_place` includes inspector_place.h.
- Each new file's header comment says what it holds; inspector_edit.h's header keeps the rules and
  points to inspector_place.h for placement.
- `editor/src/src.md` — entries for the three new files; inspector_edit's sentence.

## Done when
1. `wc -l editor/src/inspector_edit.c editor/src/inspector_place.c editor/src/inspector_buttons.c`
   shows each under 500.
2. In `p=$(mktemp -d)` with `cp -r game/example/. $p`, a capture before this card and one after
   (`build/debug/editor/voe_editor --capture <png> $p`) are equal under `cmp`.
3. `checks.sh --folder editor` prints `FINDINGS: 0`.
