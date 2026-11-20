# 07 — `voe_editor --check` reads a project strictly
folder: editor
after: 06
decisions: 0168, 0509, 0511

## Change
- `editor/src/check.h` and `editor/src/check.c` are new. They provide
  `int voe_editor_check(const char *folder);`, which returns the exit code.
  - **Folder.** NULL means the current folder (`voe_platform_path_absolute`, `platform/path.h`). A folder
    with no `project.voe3d` gets one line saying it is not a project, and returns 1.
  - **World.** Start from `voe_game_world_new(arena)` (`game/world.h`). When the file at
    `voe_editor_game_tree_library(folder, arena)` exists, open it with
    `voe_editor_code_open(that, folder, 0, …)` (`editor/src/code.h`; load 0 never clashes with the
    editor's loads, which count from 1). Then call `code.register_types(world)` and close the code at
    the end. When `Code/` exists but no library is built, print one line saying the project's own
    components were not checked.
  - **Files.** Take them from `voe_editor_assets_list` and read each with `voe_platform_file_read`. Call
    `voe_base_report_error_clear()` before each, then hand the file to the strict reader the editor
    uses for its kind:
    - `.scene`: `voe_authoring_scene_check`;
    - `.prefab`: the same, then read it into a fresh world (`voe_authoring_scene_read`). It is refused
      unless exactly one entity has no parent and `voe_editor_prefab_refused` (`editor/src/prefabs.h`)
      passes;
    - `.material`: `voe_assets_material_read_strict`;
    - `.landscape`: `voe_assets_landscape_read_strict`;
    - `.parts`: `voe_assets_parts_read`;
    - `project.voe3d`: `voe_authoring_project_read_strict`.
  - **Output.** One stdout line per refused file, `<path>: <voe_base_report_error_first()>`, or the
    reason when no report was kept. Then one summary line: `all is well: N files checked`, or
    `K of N files refused`. Stderr may carry the readers' own reports.
  - **Exit.** 0 when all is well, 1 when any file is refused or the folder is not a project.
- `editor/src/main.c` (787 lines): right after the options are read, `if (options.check) return
  voe_editor_check(options.folder);`, before any window or device. Add nothing else there.
- `editor/src/src.md`, `editor/editor.md`: add entries for check.c, and for the command.

## Meaning
- Holds: there is no check command yet; the strict calls are those named above. Parts has only the
  lenient read.
- Holds: a section of an unregistered type passes the scene check, so a project's own components are
  checked only when its code is loaded.

## Done when
This bash script exits 0:
```
d=$(mktemp -d); p=$d/p; e=$PWD/build/debug/editor/voe_editor; mkdir $p
printf '[project]\nscene = "main.scene"\n' > $p/project.voe3d
printf '[1]\nname = "Sun"\n[1.voe_scene_transform]\n[1.voe_scene_light]\n' > $p/main.scene
$e --check $p > $d/o || exit 1; grep -q "all is well" $d/o || exit 1
printf '[1]\nname = "Lamp"\n[1.voe_scene_light]\ntexture = "x.png"\n' > $p/bad.scene
$e --check $p > $d/o; test $? -eq 1 || exit 1; grep bad.scene $d/o | grep -q texture || exit 1
cd $p && $e --check > /dev/null; test $? -eq 1
```
If the strict read wants more identity keys in `[1]`, add them. The script must keep its point: a
clean project passes, and a wrong key is named with its file.
