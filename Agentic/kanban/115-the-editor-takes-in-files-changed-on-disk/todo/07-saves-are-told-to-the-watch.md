# 07 — The session's and the store's writes are told to the watch
folder: editor
after: 06
decisions: 0168, 0506, 0509

## Change
The session owns a disk watch, and each write during a save tells the watch its path (0509 point 1).

- `editor/src/session.h`, `editor/src/session.c`: the session holds a
  `voe_editor_disk_watch *watch`. It is made where the session is made and destroyed where the
  session is; session.h's header says where that is. session.c is 765 lines, so add only lines
  here, and no helper longer than a few.
- `editor/src/project.h`, `editor/src/project.c`: change every function that writes a file:
  `voe_editor_project_save`, `_window_set`, `_prefab_back`, and any other that calls
  `voe_platform_file_write`.
  - Each takes a `voe_editor_disk_watch *watch` (NULL allowed).
  - After each successful write, each calls `voe_editor_disk_watch_wrote` with the path written.
  - Callers pass `session->watch`: `session.c` (save and prefab back) and `interface_read.c`
    (window_set).
- `editor/src/models.h`, `editor/src/models.c`: `voe_editor_models_save` and
  `_landscape_shape` take the watch the same way and tell it each landscape written. Callers
  `main.c` (save) and `interface_read.c` (shape) pass the session's watch.
- Headers: the comment of each changed function says it tells the watch.

## Meaning
- Scout: the editor cannot tell its own writes from outside ones.

## Done when
- `test $(grep -c voe_editor_disk_watch_wrote editor/src/project.c) -ge 3` and
  `grep -q voe_editor_disk_watch_wrote editor/src/models.c` both exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
