# 06 — The `--check` option and the list of a project's files
folder: editor
after: 05
decisions: 0168, 0509, 0511

## Change
- `editor/src/options.h` and `editor/src/options.c`:
  - add `bool check;`, set by `--check`. The positional folder stays `folder` and is optional;
  - the usage line gains the form `voe_editor --check [<folder>]`;
  - `--check` together with `--capture`, `--size`, `--frames` or `--capture-view` is a usage error,
    handled the way the file already handles one (the caller returns 2).
- `editor/src/assets_walk.h` and `editor/src/assets_walk.c`: add
  `[[nodiscard]] bool voe_editor_assets_list(const char *project, voe_base_arena *arena,
  const char *const **out, uint32_t *count, const char **why);`. It returns every file the walk sees
  through its one extension list, with the walk's skips, plus top-level `project.voe3d` when present.
  Paths are project-relative with `/` separators, sorted bytewise. The header gains an example call and
  the point that this list is the files the watch (0509) and the check see.
- `editor/src/src.md`: entries change only if a file's line no longer says what it holds.

`main.c` does not change on this card. With `check` set and nothing acting on it, the editor would open
as usual; card 07 branches on it.

## Done when
`build/debug/editor/voe_editor --check --capture x.png; test $? -eq 2 && grep -q voe_editor_assets_list editor/src/assets_walk.h`
exits 0.
