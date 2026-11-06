# 14 — The editor takes in files changed on disk
folder: editor
after: 13
decisions: 0168, 0386, 0470, 0505, 0506, 0509
read: feature.md

## Change
- `editor/src/main.c` is 771 lines: add only a few lines here and put anything more into
  `disk_take.c`. It holds a `voe_editor_disk_take`.
  - Each frame, between frames, when Play is not running and no splash wait is shown, it calls
    `voe_editor_disk_take_step`.
  - While `voe_editor_disk_take_asking` names a path, the session's disk question is open on that
    path. Open it once, and close it when no path is named.
  - The answer the session holds goes to `voe_editor_disk_take_answer`, and is then cleared.
- `editor/src/disk_take.h`, `editor/src/disk_take.c`: add a helper for the steps above if that
  keeps main.c short.
- `editor/editor.md`: the editor takes in files changed on disk (0506).

## Done when
- `grep -q voe_editor_disk_take_step editor/src/main.c` exits 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
- The human, in the editor on a project with a sun, walks through `## How to test` in `feature.md`,
  steps 1 to 7.
