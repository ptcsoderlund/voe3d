# 06 — The editor writes a project's game tree and knows how to build it
folder: editor
decisions: 0168, 0188, 0235, 0237

## Change
What Play writes and runs, as data and files; nothing presses Play yet (card 07).

- `editor/src/game_tree.h`, `editor/src/game_tree.c` — new. Every call takes the project's absolute
  folder (asserts non-NULL) and an arena for its paths and working memory.
  - `[[nodiscard]] bool voe_editor_game_tree_write(const voe_editor_project *project,
    voe_base_arena *arena, voe_editor_notice *why);` Makes `<folder>/Build/game/` as needed and
    writes three files there through `platform/file.h`, each only when the file is missing or its
    bytes differ (so a second Play rebuilds only the scene):
    - `CMakeLists.txt`: minimum 3.28, a fixed project name, `VOE_ENGINE` set to
      `VOE_TOOLCHAIN_ENGINE` from `toolchain.h` (card 04), `include(${VOE_ENGINE}/cmake/game.cmake)`;
    - `main.c`: `main` returns `voe_game_run(<the project's name as an escaped C string literal>)`;
    - `scene.c`: `voe_authoring_scene_cook(project->world, "game/scene.h", "voe_game_scene_build", …)`
      — the world as it is now, unsaved edits included (0188), `project->unsaved` untouched.
    Then, when `<folder>/.gitignore` does not exist, writes one listing `/Build/` and `/Cache/`; an
    existing one is never read or changed (0235). False with `why` built from base/report.h
    (`voe_editor_notice_from_report`) on a refused cook or write; clear the report first.
  - `bool voe_editor_game_tree_configured(const char *folder, voe_base_arena *arena);` whether
    `<folder>/Build/debug/CMakeCache.txt` exists.
  - `const char *const *voe_editor_game_tree_configure(const char *folder, voe_base_arena *arena);`
    NULL-terminated argv: `VOE_TOOLCHAIN_CMAKE -S <folder>/Build/game -B <folder>/Build/debug -G
    Ninja -DCMAKE_BUILD_TYPE=Debug`, then `-DCMAKE_C_COMPILER=`, `-DCMAKE_MAKE_PROGRAM=`,
    `-DPKG_CONFIG_EXECUTABLE=`, `-DVOE_SLANGC=`, `-DVOE_WAYLAND_SCANNER=` for each non-empty value.
  - `const char *const *voe_editor_game_tree_build(const char *folder, voe_base_arena *arena);`
    `VOE_TOOLCHAIN_CMAKE --build <folder>/Build/debug --target game`.
  - `const char *voe_editor_game_tree_program(const char *folder, voe_base_arena *arena);`
    `<folder>/Build/debug/game`, with `.exe` on Windows.
  - Header points: the layout and why generated data is only in whole ignored folders (0235); why
    each file is written only when it changes; the tools come from the editor's own build (0237);
    debug info so a debugger can attach.
- `editor/src/src.md` — `game_tree.h` and `game_tree.c` entries.

Read the headers of `authoring/scene_cook.h`, `platform/file.h`, `platform/folder.h`,
`platform/path.h`, `editor/src/project.h`, `editor/src/notice.h`, and the generated
`build/debug/generated/editor/toolchain.h`.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0.
