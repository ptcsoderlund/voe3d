# 10 — A failed build shows the compiler's errors, and the bar has Refresh
folder: editor
decisions: 0168, 0240, 0242

## Change
Point 8 of 0242's panel and point 7's button. Read the headers of `editor/src/preferences.h`
(the panel to copy), `editor/src/topbar.h`, `editor/src/interface.h`, `editor/src/session.h`,
`editor/src/play.h`, `editor/src/refresh.h` (card 09) and `platform/file.h`.

- `editor/src/errors.h`, `editor/src/errors.c` (new) — the Errors panel: a struct holding whether
  it shows and up to `VOE_EDITOR_ERRORS_LINES` (48) lines of at most 160 bytes each, cut there;
  `_show(errors, log path)` reads the file and keeps its last 48 lines (an unreadable log keeps
  one line saying so); `_hide`; `_draw` as preferences' panel, anchored over the dock below the
  bar: a title, the lines in a scroll area, Close; `_clicks_read` says whether Close fired. The
  header: what it shows and why only the tail (the first error is usually near the end of a
  Ninja log, and the whole log is in `Build/build.log`), its limits and the budget it fits.
- `editor/src/session.h`, `editor/src/session.c` — an `errors` field; a refresh that FAILED, or a
  play whose configure or build failed, shows it from the log; a refresh or Play that starts hides
  it. The header says so.
- `editor/src/play.h`, `editor/src/play.c` — `voe_editor_play_poll` returns true when a configure
  or build step failed on this poll, so the session can show the panel.
- `editor/src/topbar.h`, `editor/src/topbar.c` — a Refresh button between Play and Preferences,
  its label passed in like Play's (`voe_editor_refresh_label`), firing
  `VOE_EDITOR_COMMAND_REFRESH`; the header's button count and list.
- `editor/src/interface.h`, `editor/src/interface.c` — the errors panel drawn and read where
  preferences' is, never on a frame the browser shows; Escape hides it as it does preferences
  (wherever that is read); the NODES, ELEMENTS and SCROLLS budgets and their worked sums raised by
  what the button and the panel add.
- `editor/src/src.md` — entries for errors.h and errors.c.

## Done when
1. `checks.sh --folder editor` prints `FINDINGS: 0`.
2. In `q=$(mktemp -d)` with `cp -r game/example/. $q` and `echo '#error broken' >>` the first `.c`
   in `$q/Code`: `build/debug/editor/voe_editor --capture $q/shot.png $q` exits 0; the same with a
   good copy into `$p/shot.png`; `cmp -s $p/shot.png $q/shot.png` exits non-zero.
3. The human's: open `$q/shot.png` and see the Errors panel holding the `#error broken` line, and
   `$p/shot.png` with Refresh in the bar.
