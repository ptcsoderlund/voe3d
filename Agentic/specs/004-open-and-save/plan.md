# 004 Open and save — plan

The engine folders gain what a saved project needs, each under its own tests: `base` keeps the first
error reported so a notice can show it, `platform` reads files, replaces them atomically, lists and
makes folders, works out paths and delivers typed text, `authoring` reads and writes
`project.voe3d`, and `3d` gains a saved cube shape. The editor then keeps "the project being worked on" in its own arena,
so New and Open build a whole new world beside the old one and swap only on success. Its top bar,
notices, unsaved mark and refuse-once rule live in an editor session. Its file browser is an
overlay composed from existing `ui` widgets. No new folder and no new dependency edge.

## Decisions

- The first error reported since a clear is kept per thread and readable — the editor's notices
  show the line the readers already write, and no reader's signature changes. Project-wide:
  `Agentic/decisions/0160-a-program-can-read-back-the-first-error-reported-since-it-last-asked.md`.
- Typed text is UTF-8 per poll, the Wayland keymap read in-house, no `xkbcommon` — rule 5, and
  closes D-245. Project-wide:
  `Agentic/decisions/0161-typed-text-arrives-per-poll-and-the-wayland-keymap-is-read-in-house.md`.
- `platform` owns folder listing, folder creation, home and settings folders and path arithmetic,
  and `voe_platform_file_write` writes `<path>.partial` then renames it over the file — an
  interrupted save leaves the old file whole. Project-wide:
  `Agentic/decisions/0162-platform-owns-folders-and-paths-and-a-file-write-replaces-its-file-whole.md`.
- A cube survives a save as `voe_3d_shape { kind = 1 }`, a described component in `3d`, turned
  into a mesh and material by a shape system — mesh and material are runtime-only. Project-wide:
  `Agentic/decisions/0163-a-built-in-shape-is-a-described-component-in-3d.md`.
- `project.voe3d` is `[project]` with `scene = "main.scene"`. The name is the folder's. The code
  lives in `authoring`. The last project is one line in `<settings>/voe3d/last_project`. There is
  no `voe_editor new`. Project-wide:
  `Agentic/decisions/0164-the-editor-opens-and-saves-projects-itself-and-the-project-file-lives-in-authoring.md`.
- The scene's light becomes an authored entity (`voe_scene_light`, identity "Light"). The
  editor's fixed sun in `view.c` goes, and each view's pass takes the world's first light. A world
  with no light is drawn with a light of zero intensity. The spec puts the light in the scene, so
  it is saved.
- The project being worked on owns its arena: the world, its kept sections and its folder path.
  New and Open make a fresh one, and the old one is destroyed only after the new one loaded. That
  is "the editor stays on what it had" without any undo, and it is ADR-0152 point 4's "discard the
  world" done by destroying an arena.
- Refuse-once is one armed command in the session: close, New or Open with unsaved changes is
  refused and armed. The same command again goes ahead. Anything else done — another command, a
  browser action, an inspector edit — disarms it and clears the notice. That is the spec's "a
  notice stays until the next thing is done".
- The browser is modal: while it shows, the top bar's commands and shortcuts are ignored, and the
  dock's panels and views get no pointer. Clicking a row enters that folder, and the confirm button
  acts on the folder the browser is in. It remembers its folder for the session only, and starts
  in the home folder.
- A scene-view camera and the selection are not part of the project. Cameras keep their place
  across New and Open, and the selection is cleared.
- `--capture` never writes `last_project`. It draws what a start would have opened: the
  command-line folder, else the last project, else untitled.

## Folders

- `base/` — changed — `voe_base_report_error_clear`, `voe_base_report_error_first`.
- `platform/` — changed — `voe_platform_file_read`, `voe_platform_file_exists`,
  `voe_platform_file_write` made atomic. New `platform/folder.h`: `voe_platform_folder_list`,
  `_create`, `_home`, `_settings`. New `platform/path.h`: `voe_platform_path_join`, `_parent`,
  `_name`, `_absolute`. `input.h` gains `voe_platform_input_text` and keys N, O, BACKSPACE, ENTER.
  `window.h` gains `voe_platform_window_close_refuse`. Internal keymap reader in `src/keymap.*`.
- `authoring/` — changed — new `authoring/project.h`: `voe_authoring_project_read`,
  `voe_authoring_project_write`, `VOE_AUTHORING_PROJECT_FILE`.
- `3d/` — changed — new `3d/shape_component.h` (`voe_3d_shape`, register, add, reads) and
  `3d/shape_system.h` (`voe_3d_shapes`, `voe_3d_shapes_upload`, `voe_3d_shape_system_run`, the
  capacity constants).
- `editor/` — changed — internal only (a program): `cube.*` removed; `project.*`,
  `last_project.*`, `notice.*`, `session.*`, `topbar.*`, `browser.*` added; `scene.*`, `view.*`,
  `interface.*`, `inspector.*` and `main.c` changed. The command line gains `<folder>`.

## Verification

- `cmake -P check.cmake` — exits zero on Linux (criterion 13).
- Criterion 12, broken scene on the command line:
  `d=$(mktemp -d) && printf '[project]\nscene = "main.scene"\n' > $d/project.voe3d && printf '[1]\nname = "Cube"\n[1.voe_scene_transform]\nposition = [0, 0]\nrotation = [0, 0, 0, 1]\nscale = [1, 1, 1]\n' > $d/main.scene && cp $d/main.scene $d/before && ! XDG_CONFIG_HOME=$d/cfg ./build/debug/editor/voe_editor $d 2> $d/err && grep 'main.scene' $d/err | grep -q 'line 4' && cmp -s $d/main.scene $d/before && test ! -e $d/cfg`.
  Exits non-zero before any window, names the file and line 4, and changes no file.
- Criterion 12, not a project: `d=$(mktemp -d) && ! ./build/debug/editor/voe_editor $d 2> $d/err && grep -q 'project.voe3d' $d/err`.
- Criterion 11, lost last project:
  `d=$(mktemp -d) && mkdir -p $d/cfg/voe3d && echo /nonexistent/gone > $d/cfg/voe3d/last_project && XDG_CONFIG_HOME=$d/cfg ./build/debug/editor/voe_editor --capture $d/shot.png 2> $d/err && test -s $d/shot.png && grep -q gone $d/err && test "$(cat $d/cfg/voe3d/last_project)" = /nonexistent/gone`.
  Starts on untitled, reports the lost project and rewrites nothing. Needs a graphics card.
- Criteria 1, 6 and `--capture`, a hand-made project opens:
  `d=$(mktemp -d) && printf '[project]\nscene = "main.scene"\n' > $d/project.voe3d && printf '[1]\nname = "Cube"\n[1.voe_3d_shape]\nkind = 1\n[1.voe_scene_transform]\nposition = [0.5, 0, 0]\nrotation = [0, 0, 0, 1]\nscale = [1, 1, 1]\n\n[2]\nname = "Light"\n[2.voe_scene_light]\ndirection = [0, -1, 0]\ncolour = [1, 1, 1]\nintensity = 3\n' > $d/main.scene && mkdir -p $d/cfg/voe3d && echo $d > $d/cfg/voe3d/last_project && XDG_CONFIG_HOME=$d/cfg ./build/debug/editor/voe_editor --capture $d/shot.png && test -s $d/shot.png`.
  Needs a graphics card.
- Criteria 1–10 as a person uses them — the top bar, shortcuts, unsaved mark, browser, first save,
  save and reopen, New, refuse-once on close, New and Open, and a broken project from Open — are the
  sponsor's hands-on acceptance on Linux: `./build/debug/editor/voe_editor`, with `~/.config/voe3d/`
  removed first for criterion 1.
