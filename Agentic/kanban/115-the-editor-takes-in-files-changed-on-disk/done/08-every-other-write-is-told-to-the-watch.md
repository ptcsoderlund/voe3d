# 08 — Every other editor write is told to the watch
folder: editor
after: 07
decisions: 0168, 0506, 0509

## Change
After each successful write of a project file below, call `voe_editor_disk_watch_wrote` with its
path (0509 point 1). A function with no watch in reach takes a `voe_editor_disk_watch *watch` (NULL
allowed), and its callers pass the session's.

- `material_steps.h/.c`: `voe_editor_material_step_write` and `_apply`.
  - Their callers are `frame_commands.c` and `undo.c`.
  - `voe_editor_undo_take` (`undo.h`) takes the watch so it can pass it on. Its callers
    (`frame_commands.c` and any other) pass the watch.
- `levels_ask.h/.c`: `voe_editor_levels_ask_save`. Its caller is `interface_assets.c`.
- `model_import.h/.c`: `voe_editor_model_import` and `_import_textures`, for each `.parts` and
  `.material` they write. Callers:
  - `assets_import.c`. Its `voe_editor_assets_import` (`assets_panel.h`) takes the watch, and its
    caller is `session.c`.
  - `interface_assets.c`.
- `prefab_make.c` (`prefabs.h`): `voe_editor_prefab_make`. Its caller is `interface_read.c`.
- `assets_walk.h/.c`: `voe_editor_assets_follow_write`, for each file it rewrites. Its caller is
  `assets_manage.c`, which also tells the watch after its own two `voe_platform_file_write` calls.
- Headers: the comment of each changed function says it tells the watch.

## Meaning
- Scout: the editor cannot tell its own writes from outside ones.
- Scout: materials and the project file are never unsaved; an edit writes them at once.

## Done when
- `grep -l voe_editor_disk_watch_wrote editor/src/{material_steps,levels_ask,model_import,prefab_make,assets_walk,assets_manage}.c | wc -l`
  prints 6.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
