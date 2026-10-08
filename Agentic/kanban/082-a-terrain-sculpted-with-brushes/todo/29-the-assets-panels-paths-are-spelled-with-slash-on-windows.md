# 29 — The Assets panel's paths are spelled with / on Windows
folder: editor/src
after: 27, 28
decisions: 0168, 0381, 0382, 0378

## Change
Bug 02's other half: on Windows the Assets panel must move, rename and
delete as on Linux. Its project-relative paths are `/` between
(`assets_panel.h`, `assets_manage.h`), and the scene texts a move follows
spell them so, but some are built with `voe_platform_path_join`, which writes
`\` on Windows (`platform/include/platform/path.h`, SEPARATORS). A move's
`to` or a row's path then holds a `\`, the follow writes it into scene text,
and the into-itself check, which looks for `/`, misses. Fix it once, in the
Assets files that build relative paths.

Files: `editor/src/assets_panel.h`, `editor/src/assets_panel.c`,
`editor/src/assets_manage.h`, `editor/src/assets_manage.c`,
`editor/src/src.md` (only if an entry stops being true).

- `assets_panel.h/.c`: add `voe_editor_assets_join(arena, folder, name)`
  returning `const char *`: `folder/name` with one `/` into arena, `name`
  alone when `folder` is "". Its comment: relative paths are always `/`
  between, on both platforms, because scene text spells them so;
  `voe_platform_path_join` is for a path handed to the OS.
- In `assets_panel.c` and `assets_manage.c`, every join of a relative path
  uses it: the `folder`/`name` joins in `voe_editor_assets_folder_make`,
  `voe_editor_assets_landscape_make` and `voe_editor_assets_duplicate`; in the
  panel, the listing's next folder and name and `Assets/` with a row's path
  (the `opened` and `landscape_opened` paths). A join that starts from the
  project folder and is handed to `platform/file.h`, `folder.h` or `trash.h`
  keeps `voe_platform_path_join`. Check every `voe_platform_path_join` in the
  two files against that rule; `assets_manage.c` includes `assets_panel.h`
  if it does not.
- `assets_manage.c`, `voe_editor_assets_trash`: the UNSUPPORTED notice says
  what is true on both platforms (another file system on Linux, a drive with
  no Recycle Bin on Windows) without naming either.
- `assets_manage.h`: the CONSTRAINTS line "Trash is the home trash only"
  and `voe_editor_assets_trash`'s comment name the desktop's trash, home
  trash or Recycle Bin, per `platform/trash.h`.

## Done when
- `grep -c "voe_platform_path_join" editor/src/assets_manage.c` prints `1`
  (the project folder join for disk).
- `grep -n "voe_editor_assets_join" editor/src/assets_panel.h` shows the
  declaration, and `grep -c "voe_editor_assets_join" editor/src/assets_manage.c`
  prints at least `3`.
- `grep -n "home trash only" editor/src/assets_manage.h` prints nothing.
