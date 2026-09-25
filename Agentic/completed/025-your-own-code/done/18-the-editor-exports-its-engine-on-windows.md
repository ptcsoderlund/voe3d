# 18 — On Windows the editor exports its engine through a generated `.def`
folder: editor
decisions: 0168, 0175, 0242, 0243, 0245

## Change
Point 1 of 0245. Read the headers of `voe_executable` and `voe_editor_toolchain` in
`cmake/voe.cmake` (build files `--folder` allows beside a card naming a decision).

- `cmake/exports.cmake` (new, run with `cmake -P`) — inputs `-DNM=`, `-DARCHIVES=` (a `;` list),
  `-DOUT=`. Runs `NM -A -P -g` on each archive (lines `archive[member]: name type value size`,
  the form both `llvm-nm` and GNU `nm` print), groups by member, skips every member that lists
  `voe_game_project_register` or `voe_game_project_systems_run` as `U`, and writes to `OUT`
  `EXPORTS` then one line per defined `voe_` symbol: type `T` bare, `D`, `B`, `R` or `C` with
  ` DATA`; each name once, sorted. `OUT` is written only when its bytes change. A failed `NM` is a
  `FATAL_ERROR` naming the archive. Header comment: what it writes and for whom (0245), why data
  is `DATA`, why those members are skipped, the input variables.
- `cmake/voe.cmake`, `voe_executable`, the `editor` branch — on `WIN32` only: an
  `add_custom_command` whose OUTPUT is `${CMAKE_CURRENT_BINARY_DIR}/voe_editor.def`, running
  `exports.cmake` with `CMAKE_NM` and `$<TARGET_FILE:voe_<dep>>` for every DEPENDS folder,
  `game` included, DEPENDS on those same files and the script; the `.def` added to
  `voe_editor`'s sources (CMake passes a `.def` source to the link, and the link writes the
  import library `voe_editor.lib`). A missing `CMAKE_NM` on `WIN32` is a `FATAL_ERROR`. Linux is
  unchanged. The function's header gains the Windows half: the `.def`, why (a DLL binds only to
  exported names, 0245), and that `game`'s `run.c` stays out.

## Done when
1. `checks.sh --folder editor` prints `FINDINGS: 0`.
2. With `t=$(mktemp -d)` and `a` the `;`-joined `build/debug/{base,ecs,scene,game}/libvoe_*.a`:
   `cmake -DNM=llvm-nm "-DARCHIVES=$a" -DOUT=$t/e.def -P cmake/exports.cmake` exits 0, and
   `$t/e.def` starts with `EXPORTS`, holds `voe_ecs_component_register` and
   `voe_game_project_component` bare, `voe_scene_transform_key DATA`, and neither `voe_game_run`
   nor `voe_game_project_register`.
3. `-DNM=nm` gives the same file.
