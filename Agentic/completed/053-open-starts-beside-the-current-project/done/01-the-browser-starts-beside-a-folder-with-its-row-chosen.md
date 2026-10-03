# 01 — The browser starts beside a folder it is given, with that folder's row chosen
folder: editor/src
after: none
decisions: 0168, 0343

## Change
0343 points 1, 3 and 4, the browser's half. Read `editor/src/browser.h`, `editor/src/browser.c`,
`editor/src/session.c`, `editor/src/interface.c` and `editor/src/src.md`.

- `browser.h`, struct `voe_editor_browser`: add the chosen row (an index into `rows`, with a value
  meaning none) and `const char *target`, absolute, in `arena`: the folder Open's Confirm acts on,
  the chosen row's path while one is chosen in OPEN mode, else `folder`.
- `voe_editor_browser_show(browser, mode, const char *beside, why)`: `beside` is an absolute
  project folder or NULL, copied, never kept by pointer. Given, its parent
  (`voe_platform_path_parent`) is listed through the same `relist` path as today, and when a row's
  name equals `voe_platform_path_name(beside)` that row is chosen. Given but the parent will not
  list or no row matches, start as a first showing does (home, else "."), nothing chosen. NULL:
  today's rule unchanged.
- `relist` (the one place every navigation lands): a successful relist clears the chosen row and
  sets `target` to the new `folder`; a failed one changes neither. `show` chooses the row after its
  relist, and sets `target` to it in OPEN mode only.
- `voe_editor_browser_draw`: the chosen row is drawn with `voe_ui_choice_begin(ui, "row", i,
  true)`, every other row as now (or all rows through `voe_ui_choice_begin` with `i == chosen`).
  `clicks_read` reads the rows as now; a press still enters.
- Callers, so the tree builds: the two `voe_editor_browser_show` calls in `session.c` (OPEN and
  SAVE) and the IMPORT one in `interface.c` pass NULL. Card 02 fills session's.
- Headers, `browser.h`: the top comment's "keeps its folder across showings" names `beside` as the
  exception and that Import passes none; a new point: a showing beside a folder starts in its
  parent with it chosen, why (Open, Open again reopens it), a missing one falls back to home, and
  navigation clears the choice; `clicks_read`'s "Confirm names no folder of its own" now points at
  `target`; `show`'s comment states `beside`'s three cases; the struct fields say what they hold.
- `src.md`: `browser.h`'s entry names the start beside a folder and the chosen row.

## Done when
`grep -c "voe_ui_choice_begin" editor/src/browser.c` prints 1 or more,
`grep -c "beside" editor/src/browser.h` prints 1 or more, and the folder builds.
