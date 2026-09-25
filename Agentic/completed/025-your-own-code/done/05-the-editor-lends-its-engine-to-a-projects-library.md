# 05 — The editor lends its own engine to a project's library
folder: editor
decisions: 0168, 0175, 0008, 0242

## Change
Point 4 of 0242, second half, and the one caller of card 01's new `voe_platform_process_start`.
Read the header of `voe_executable` in `cmake/voe.cmake` (a build file `--folder` allows beside
a card naming a decision).

- `cmake/voe.cmake`, `voe_executable`, inside its `folder STREQUAL "editor"` branch —
  `ENABLE_EXPORTS ON` for `voe_editor`, and every dep in `DEPENDS` but `game` linked whole-archive
  (`$<LINK_LIBRARY:WHOLE_ARCHIVE,...>`; where CMake refuses a library linked both with and without
  the feature, use the `LINK_LIBRARY_OVERRIDE_<library>` property instead). The function's header
  gains why: a project library's engine symbols bind to the editor's, so every engine function must
  be in the program and exported (0242 point 4); `game` is left out because its entry points
  (`game/project.h`) are the project's to define. `voe_dev` is unchanged.
- `editor/src/play.c` — `play_step` passes NULL as the output path, keeping 024's shared output
  until card 08 sends it to the log. No other change.
- `editor/editor.md` — nothing changes unless an entry names how the editor is linked.

## Done when
1. `checks.sh --folder editor` prints `FINDINGS: 0`.
2. `nm -D --defined-only build/debug/editor/voe_editor` lists `voe_ecs_component_register`,
   `voe_platform_input_key_down` and `voe_scene_transform_submit`, and does not list
   `voe_game_project_register`.
3. In `p=$(mktemp -d)`: `cp -r game/example/. $p`, then
   `build/debug/editor/voe_editor --capture $p/shot.png $p` exits 0 and `$p/shot.png` exists
   (card 04's data opens).
