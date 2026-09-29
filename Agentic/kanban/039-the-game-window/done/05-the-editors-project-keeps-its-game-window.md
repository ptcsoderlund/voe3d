# 05 — The editor's project keeps its game window
folder: editor
after: 01, 04
decisions: 0168, 0291, 0164

## Change
0291 points 1 and 4. Card 01 gave `voe_authoring_project` a `window`
(`voe_authoring_project_window`, defaults and range macros in `authoring/project.h`). Files:
`editor/src/project.h`, `editor/src/project.c`, `editor/src/src.md`.

- `voe_editor_project` gains the project file as read: its `scene` path (copied into the
  project's arena) and its `window`. An untitled project has `main.scene` and the default window.
  Opening reads both from `project.voe3d`; a code swap, a scene set and a prefab keep them.
- Saving an untitled project writes `project.voe3d` with the project's window, where it writes
  `{ .scene = SCENE_FILE }` today.
- New: `[[nodiscard]] bool voe_editor_project_window_set(voe_editor_project *project,
  voe_authoring_project_window window, voe_editor_notice *why);` — asserts a size in range; sets
  the window; with a folder, writes `project.voe3d` at once through `platform/file.h` (the
  project's scene path kept); untitled writes nothing. False with why when the write fails, the
  window still set. It never touches `unsaved` or the undo line.
- Header points: the project file is kept as read so a settings write keeps its scene path; the
  window is a project setting written at once, not a scene edit (0291 point 4); an untitled
  project writes it with its first save.
- `src.md`: the `project.h` and `project.c` entries name the game window.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" &&
test -s "$d/t.png"` exits 0 (a project file with no `[window]` still opens).
