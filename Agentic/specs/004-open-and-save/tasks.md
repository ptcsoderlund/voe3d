# 004 Open and save — tasks

- [x] 1. `base/` — Keep the first error reported since a clear
  - Change: In `include/base/report.h` and `src/report.c`, per ADR-0160, add
    `void voe_base_report_error_clear(void);` and `const char *voe_base_report_error_first(void);`.
    `voe_base_report_at` at `VOE_BASE_LEVEL_ERROR`, when nothing is kept since the last clear,
    copies the composed `<message>` part into a `thread_local` fixed buffer and sets a
    `thread_local` flag. The copy has no level word, no module and no newline, and is cut at the
    printed line's capacity. `_first` returns that buffer, or NULL when nothing is kept. Warnings
    are never kept. Printing to stderr is unchanged. The header gains a paragraph: what is kept,
    why the first, why per thread, and that the reader clears before the operation it wants to
    explain. If `src/report_line.h` is the composition, reuse it. Extend `tests/report.c`: nothing
    kept after a clear, the first of two errors kept, a warning not kept, a clear forgets, and a
    long message is cut at the capacity. Update `base/base.md`'s `report.h` line.
  - Covers: 10, 11, 12 (the message a notice or the command line shows)
  - Depends on: -
  - Done when: `cmake --build --preset debug && ctest --test-dir build/debug -R '^base/'` passes and `cmake -P check.cmake` exits 0

- [x] 2. `platform/` — Read a whole file, test for one, and make the write atomic
  - Change: Per ADR-0162, in `include/platform/file.h` add
    `[[nodiscard]] const uint8_t *voe_platform_file_read(const char *path, voe_base_arena *arena, size_t *out_count, voe_base_error *error);`.
    It returns the whole file pushed into `arena` with a NUL after the last byte, which
    `*out_count` does not count. An empty file is a valid pointer and a count of 0. NULL with
    `VOE_BASE_ERROR_UNAVAILABLE` means the path cannot be opened: missing, a folder, permission.
    NULL with `VOE_BASE_ERROR_REFUSED` means a read failed after opening. Each failure is
    reported at the site with the path and the OS reason, through `base/report.h`. Also add
    `bool voe_platform_file_exists(const char *path);` — true only for a regular file. Change
    `voe_platform_file_write` to write `<path>.partial`, flush it (`fsync` / `FlushFileBuffers`),
    close it and rename it over `path` (`rename` / `MoveFileExA` with
    `MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH`). On any failure after the partial file
    was created, delete it and return `REFUSED`, leaving `path` untouched. An unopenable partial
    still returns `UNAVAILABLE` and creates nothing. Rewrite the header's promises to match. Both
    backends: `src/file_wayland.c` and `src/file_win32.c`. On Windows there is no `fopen` and no
    `getenv` in `src/` (ADR-0159). Extend `tests/file.c`: read back what was written, an empty
    file, a missing path, a folder refused, no `.partial` left after a success, and a write whose
    `path` is an existing folder — the rename fails, `REFUSED`, the folder is still a folder and
    no `.partial` is left. Update `platform/platform.md`.
  - Covers: 5, 6, 10 (reading a scene); "an interrupted save leaves the previously saved file whole"
  - Depends on: 1
  - Done when: `cmake --build --preset debug && ctest --test-dir build/debug -R '^platform/file$'` passes and `cmake -P check.cmake` exits 0

- [x] 3. `platform/` — Folders: list, make, home, settings
  - Change: Per ADR-0162, add `include/platform/folder.h` with
    `typedef struct { const char *name; bool folder; bool hidden; } voe_platform_folder_entry;`,
    `typedef struct { const voe_platform_folder_entry *entries; uint32_t count; } voe_platform_folder_listing;`
    and these calls:
    `[[nodiscard]] bool voe_platform_folder_list(const char *path, voe_base_arena *arena, voe_platform_folder_listing *out, voe_base_error *error);`
    lists every entry except `.` and `..`, names in the arena, sorted by byte order, hidden = a
    leading `.` on Linux or `FILE_ATTRIBUTE_HIDDEN` on Windows. `folder` follows `d_type`, with
    `fstatat` when that is `DT_UNKNOWN`, and a symlink to a folder counts as a folder.
    `[[nodiscard]] bool voe_platform_folder_create(const char *path, voe_base_error *error);`
    makes one level, `UNAVAILABLE` when the parent is missing or not allowed, `REFUSED` when
    something already has that name.
    `const char *voe_platform_folder_home(voe_base_arena *arena);` is `$HOME` on Linux,
    `USERPROFILE` on Windows, and NULL when unset.
    `const char *voe_platform_folder_settings(voe_base_arena *arena);` is `$XDG_CONFIG_HOME`
    when it is set and absolute, else `$HOME/.config` on Linux, `APPDATA` on Windows, and NULL
    when unknown.
    No trailing separator on any returned path. Sources: `src/folder_wayland.c` and
    `src/folder_win32.c`; Windows reads the environment with `GetEnvironmentVariableA`. Test
    `tests/folder.c`, needing no display: make a scratch folder under the test's working
    directory, a nested folder, a file and a dotted file in it, then check the listing's order
    and flags. An empty folder lists zero. Creating over an existing name is `REFUSED`, under a
    missing parent `UNAVAILABLE`. Settings honours `XDG_CONFIG_HOME` set by the test with
    `setenv`, which is Linux-only and may be guarded by `#ifndef _WIN32` in the test. Update
    `platform/platform.md`.
  - Covers: 4, 6, 7, 11 (browsing, emptiness, where the last project is kept)
  - Depends on: 2
  - Done when: `cmake --build --preset debug && ctest --test-dir build/debug -R '^platform/folder$'` passes and `cmake -P check.cmake` exits 0

- [x] 4. `platform/` — Paths: join, parent, name, absolute
  - Change: Per ADR-0162, add `include/platform/path.h`:
    `const char *voe_platform_path_join(voe_base_arena *arena, const char *folder, const char *name);`
    puts exactly one separator between, and none extra after a root.
    `const char *voe_platform_path_parent(voe_base_arena *arena, const char *path);` returns NULL
    at a root (`/`, `C:\`), and a trailing separator is ignored.
    `const char *voe_platform_path_name(const char *path);` returns a pointer into `path` at its
    last name, and `""` for a root.
    `[[nodiscard]] const char *voe_platform_path_absolute(const char *path, voe_base_arena *arena, voe_base_error *error);`
    resolves against the working directory with symlinks and `..` resolved, `realpath` on Linux
    and `GetFullPathNameA` plus an existence check on Windows. NULL with `UNAVAILABLE` and a report
    naming the path when nothing is there.
    Separators are `/` on Linux, and `\` or `/` on Windows, which writes `\`. Put the string
    arithmetic in `src/path.c` for both platforms, with the one separator decision behind a
    compile-time constant rather than an `#ifdef` in each function. Only `_absolute` goes in
    `src/path_wayland.c` and `src/path_win32.c`. Test `tests/path.c`, needing no display: join,
    parent and name on ordinary paths, roots and trailing separators, for the platform it runs
    on; absolute of `.` equals itself made absolute twice, and of a missing path is NULL. Update
    `platform/platform.md`.
  - Covers: 4, 6, 7, 12 (a folder named on the command line, the project name, going up)
  - Depends on: 3
  - Done when: `cmake --build --preset debug && ctest --test-dir build/debug -R '^platform/path$'` passes and `cmake -P check.cmake` exits 0

- [x] 5. `platform/` — The XKB keymap reader
  - Change: Per ADR-0161, add `src/keymap.h` and `src/keymap.c`, internal and built on both
    platforms because they include no OS header:
    `#define VOE_PLATFORM_KEYMAP_CODES 256`, and
    `typedef struct { uint32_t typed[VOE_PLATFORM_KEYMAP_CODES][2]; } voe_platform_keymap;`,
    indexed by evdev code (the XKB keycode minus 8) and level (0 plain, 1 shifted), where 0
    means the key types nothing. Also
    `bool voe_platform_keymap_read(const char *text, size_t size, voe_platform_keymap *out);`.
    It reads the resolved XKB v1 text a compositor sends. From the `xkb_keycodes` block it takes
    `<NAME> = N;` and `alias <A> = <B>;`. From the `xkb_symbols` block it takes each
    `key <NAME> { … }` in both spellings: a bare `[ sym, sym, … ]`, and
    `symbols[Group1] = [ … ]` with `type = "…"` beside it. Group 1 only, levels 1 and 2. A key
    with one level types that level shifted too. It skips every other block and statement. A
    keysym name becomes a code point by a table of the X11 Latin-1 names (0x20–0x7e, 0xa0–0xff:
    `space`, `exclam`, `a`, `A`, `odiaeresis`, `aring`, …), `U` followed by hex, and `0x0100`
    followed by hex. Any other name is 0. It returns false and leaves `*out` zeroed when there is
    no symbols block; the Wayland backend reports that, not this file. Use a plain forward
    tokenizer with fixed limits — name length, keycode names — and no recursion, and say why in
    the header (rule 14's reasoning: the text comes from outside the program). Name what is not
    read: Caps Lock, level 3 / AltGr, dead keys, compose, other groups. Test `tests/keymap.c`:
    embed a trimmed keymap text holding `<AC01>` a/A, `<AC10>` odiaeresis/Odiaeresis with a
    type, `<AE01>` 1/exclam, `<SPCE>` space with one level, an alias, a dead key and a
    `U20AC`. Check each code and level, that the dead key types nothing, and that text with no
    symbols block is refused. Update `platform/platform.md`.
  - Covers: 4 (a typed folder name, on Linux)
  - Depends on: 4
  - Done when: `cmake --build --preset debug && ctest --test-dir build/debug -R '^platform/keymap$'` passes and `cmake -P check.cmake` exits 0

- [x] 6. `platform/` — Typed text, four keys, and a refused close
  - Change: Per ADR-0161, `include/platform/input.h` gains
    `typedef struct { const char *bytes; uint32_t size; } voe_platform_text;` and
    `voe_platform_text voe_platform_input_text(voe_platform_window *window);`. It returns the
    UTF-8 typed since the previous poll, in order, valid until the next poll. Keys
    `VOE_PLATFORM_KEY_N`, `_O`, `_BACKSPACE` and `_ENTER` join the enum and both backends' tables.
    In `src/input.h` and `src/input.c`, the input state gains a fixed text buffer (256 bytes) and
    a size, and one shared function appends a code point as UTF-8. It drops code points below
    0x20, 0x7f, surrogates, anything past 0x10FFFF, and a code point that would not fit whole.
    The poll's clear empties the buffer, and so does losing focus.
    `src/window_wayland.c` keeps the keymap fd: it `mmap`s it read-only and private, runs
    `voe_platform_keymap_read` into the window, unmaps and closes. A refused keymap is reported
    once and types nothing. On a key press with Control not held, it appends
    `typed[code][shift ? 1 : 0]` when non-zero. `src/window_win32.c` handles `WM_CHAR`, joining a
    UTF-16 surrogate pair and dropping everything while Control is held without Alt (AltGr is
    Control+Alt and still types). Rewrite the headers' "no text" and "no xkbcommon" / "no
    WM_CHAR" paragraphs; do not append to them.
    `include/platform/window.h` gains `void voe_platform_window_close_refuse(voe_platform_window *window);`:
    it clears `should_close`. A Wayland connection that has died sets it again at the next poll,
    and the header says so. Both backends. Extend `tests/input.c`: typed bytes survive until the
    poll and are gone after it, a focus loss empties them, a code point that does not fit is
    dropped whole, and control code points are dropped. Update `platform/platform.md`.
  - Covers: 2 (Ctrl+N, Ctrl+O), 4 (typing a name), 9 (a refused close)
  - Depends on: 5
  - Done when: `cmake --build --preset debug && ctest --test-dir build/debug -R '^platform/'` passes and `cmake -P check.cmake` exits 0; verified on Linux, Windows backend written and unverified

- [x] 7. `authoring/` — The project file
  - Change: Per ADR-0164, add `include/authoring/project.h` and `src/project.c`:
    `#define VOE_AUTHORING_PROJECT_FILE "project.voe3d"`,
    `typedef struct { const char *scene; } voe_authoring_project;`,
    `[[nodiscard]] bool voe_authoring_project_read(const char *text, size_t size, voe_base_arena *arena, voe_authoring_project *out);`
    and
    `const char *voe_authoring_project_write(const voe_authoring_project *project, voe_base_arena *arena, size_t *out_size);`
    The reader goes through `assets/sectioned.h`. It refuses the following, each reported through
    `VOE_BASE_ERROR("authoring", "line %u: …")` with the line where one exists: a malformed file
    (the parser's own report); a section other than `[project]`; no `[project]`; no `scene` key;
    a `scene` that is empty, starts with `/`, has a drive letter, contains `\` or a control
    character, or has an empty, `.` or `..` name between its `/`s. An unknown key in `[project]`
    is a warning with its line, and the file still loads. Take line numbers the way
    `src/scene_read.c` does; if that means a second copy of its line walk, move the walk into a
    shared `src/` file used by both instead. The writer emits exactly
    `[project]\nscene = "<scene>"\n` with `"` and `\` escaped, and asserts that the path is one
    the reader accepts. It cannot fail. Test `tests/project.c`: the exact bytes written, read back
    to the same `scene`, every refusal returning false with the line in
    `voe_base_report_error_first()` where one exists, and an unknown key loading with a warning.
    Update `authoring/authoring.md`, whose opening sentence now covers the project file.
  - Covers: 4, 5, 6, 7 (a folder is a project because its `project.voe3d` reads), 10
  - Depends on: 1
  - Done when: `cmake --build --preset debug && ctest --test-dir build/debug -R '^authoring/'` passes and `cmake -P check.cmake` exits 0

- [ ] 8. `3d/` — The built-in shape and its system
  - Change: Per ADR-0163, add `include/3d/shape_component.h` and `src/shape_component.c`. The
    component is `#define VOE_3D_SHAPE_FIELDS(F, F_READ_ONLY) F_READ_ONLY(uint32_t, kind, UINT32)`,
    `VOE_BASE_DESCRIBE_STRUCT(voe_3d_shape, …)`, `#define VOE_3D_SHAPE_CUBE 1u`,
    `extern const struct voe_ecs_key voe_3d_shape_key;`, `void voe_3d_shape_register(voe_ecs_world *world, uint32_t capacity);`
    (described, no replace intent, the compiled-out marker in the `#else` branch as other
    described types do), `[[nodiscard]] bool voe_3d_shape_add(voe_ecs_world *world, voe_ecs_entity entity, voe_3d_shape shape);`,
    and `_get`, `_count`, `_rows`, `_entities` as `scene`'s light has them.
    Add `include/3d/shape_system.h` and `src/shape_system.c`:
    `typedef struct { voe_render_geometry cube; voe_3d_material material; } voe_3d_shapes;`, the
    constants `VOE_3D_SHAPES_VERTICES` (24), `VOE_3D_SHAPES_INDICES` (36),
    `VOE_3D_SHAPES_GEOMETRIES` (1) and `VOE_3D_SHAPES_SHADINGS` (1), and
    `[[nodiscard]] bool voe_3d_shapes_upload(voe_render_device *device, voe_3d_shapes *out, voe_base_error *error);`.
    The upload creates the cube geometry, then an opaque material of base colour 0.7 grey,
    metallic 0 and roughness 0.6 through `voe_3d_material_upload`. Also add
    `void voe_3d_shape_system_run(voe_ecs_world *world, const voe_3d_shapes *shapes);`: for every
    shape row whose entity has no mesh, a known kind gets `voe_3d_mesh_add` (world layer) and
    `voe_3d_material_add` with the shapes' values. An unknown kind adds nothing and warns once
    per run of unknown kinds, edge-triggered like the scene drains. The world must have mesh and
    material registered, and a full table asserts. The vertices and indices move from
    `editor/src/cube.c` into `src/cube.h` and `src/cube.c`, internal. The editor's copy is deleted
    by task 10, not here. Test `tests/shape.c`. The table half needs no graphics card: the
    registration is described and not runtime-only, `kind` is read-only, a run with a
    hand-made `voe_3d_shapes` gives a shaped entity exactly one mesh and material, a second run
    adds nothing, an entity without a shape is untouched, and an unknown kind gets nothing. The
    GPU half uploads on a headless device sized from the constants, and skips with a reason
    without a card, as `tests/material.c` does. Update `3d/3d.md`.
  - Covers: 1, 5, 6, 8 (the cube is drawn after a load)
  - Depends on: -
  - Done when: `cmake --build --preset debug && ctest --test-dir build/debug -R '^3d/'` passes and `cmake -P check.cmake` exits 0

- [ ] 9. `ui/` — A single-line text field
  - Change: Per ADR-0165, add to `include/ui/widgets.h` and `src/widgets.c`:
    `#define VOE_UI_FIELD_CAPACITY 256` — how many bytes of text a field holds, the NUL not
    counted;
    `typedef struct { const char *text; uint32_t size; bool backspace; bool enter; } voe_ui_keyboard;`
    and `void voe_ui_keyboard_set(voe_ui_context *ui, voe_ui_keyboard keyboard);` — handed over
    between `voe_ui_frame_begin` and `voe_ui_frame_end`, once, exactly as the pointer is, `text`
    being the UTF-8 typed since the previous frame and read for `size` bytes. A frame that never
    calls it has no typing, as a frame that says nothing about the pointer has none;
    `voe_ui_node voe_ui_field(voe_ui_context *ui, const char *name, uint32_t index, const char *text, voe_ui_sizing sizing);`
    `void voe_ui_field_focus(voe_ui_context *ui, voe_ui_node field);` — takes focus to that field,
    called after the field call and before the frame ends, for the frame a panel holding one opens;
    on a node that is not a field it asserts;
    `typedef struct { bool focused; bool changed; bool entered; const char *text; } voe_ui_field_result;`
    and `voe_ui_field_result voe_ui_field_action(const voe_ui_context *ui, voe_ui_node field);`,
    read after `voe_ui_frame_end` like a button's and a number box's answer.
    The field is a keyed container built as a button is — `BUTTON_PAD`, the run along START, the
    three state colours and a fourth for focused — and the call makes and ends the one label of
    `text` inside it itself, so a field costs two nodes and its `text` is read at `voe_ui_frame_end`
    and not copied, as a label's is. It takes `voe_ui_sizing` and not a whole `voe_ui_container`:
    the padding, the flow and the clipping are the widget's.
    A new `VOE_UI_WIDGET_FIELD` in `src/context.h`. Emission: the background in its state colour,
    the label's glyphs by the existing `push_label` path, and when focused a caret — one SOLID
    record `VOE_UI_FIELD_CARET` (0.3 mm) wide at the right edge of the label's rectangle, as tall as
    that rectangle, in `LABEL_INK`, clipped through `voe_ui_limit` like every other record. An empty
    text measures to nothing, so the caret sits at the left of the content box.
    Focus is one key in the context beside `held` and survives between frames: a press inside a
    field's visible rectangle focuses it, a press anywhere else clears it, and a focus whose field
    was not called this frame is dropped at `voe_ui_frame_end`, as a scroll area that is not called
    is forgotten.
    Editing happens in the resolve inside `voe_ui_frame_end`, on the focused field only, and in this
    order: Backspace removes the last code point of the handed-in text (the trailing continuation
    bytes and the byte before them) and does nothing on an empty text, then the typed bytes are
    appended code point by code point, stepping by the leading byte's own length so nothing reads
    past `size`, dropping a trailing partial sequence and dropping whole any code point that would
    not fit. The answer goes into one `char [VOE_UI_FIELD_CAPACITY + 1]` in the context — one
    buffer, because only one field can be focused. `result.text` is that buffer when `changed`, and
    the pointer handed in when not, so a caller may write it back every frame and get the same
    answer; it is valid until the next `voe_ui_frame_begin`. `entered` is true on the frame Enter
    arrived while the field was focused; Enter changes no text and moves no focus. Backspace and
    Enter with nothing focused do nothing.
    Rewrite the header's WHAT IS NOT HERE, do not append to it: text input exists for this widget
    and for nothing else, there is still no selection, no clipboard, no moving the caret, no
    multi-line and no typing into a number box, whose click stays reserved. Say why the field takes
    its text instead of composing a label in as a button does — the caret is measured from it — why
    the edited text comes back as a value rather than the caller's buffer being written, that where
    the bytes came from is not this folder's business, and what a field costs in nodes and in
    element records.
    Extend `tests/widgets.c`, which needs no window system: a press focuses and a press elsewhere
    unfocuses; typed bytes appended in order; two fields in a frame, only the focused one changing;
    Backspace taking a two-byte code point (`ö`) whole; Backspace on an empty text; a text at the
    capacity refusing the next code point whole; `entered` true only on the frame Enter arrived; a
    field not called losing focus; `voe_ui_field_focus` taking it; and `changed` false handing back
    the caller's own pointer. Only a case that measures a string takes a headless device, as the
    file already splits them. Update `ui/ui.md`.
  - Covers: 4 (typing a folder name)
  - Depends on: -
  - Done when: `cmake --build --preset debug && ctest --test-dir build/debug -R '^ui/widgets$'` passes and `cmake -P check.cmake` exits 0

- [ ] 10. `editor/` — Start on an untitled cube and light
  - Change: Replace the four built entities with the untitled scene. Entity 1 is "Cube":
    identity, a transform at the origin with no turn and scale 1, and `voe_3d_shape` kind cube.
    Entity 2 is "Light": identity and a `voe_scene_light` with direction
    normalize(-0.4, -1, -0.6), colour (1, 1, 1) and intensity π — the numbers `view.c`'s sun has
    today. Rename `voe_editor_scene_build` to `voe_editor_scene_untitled(scene, world)`. It no
    longer uploads, so it takes no device and cannot fail, and it asserts two identities. Delete
    `src/cube.h` and `src/cube.c`. In `main.c`: register `voe_scene_light` and `voe_3d_shape`, and
    raise `MAX_COMPONENT_TYPES` if needed; upload `voe_3d_shapes` once after the device opens;
    size `EDITOR_CAPACITIES` from `VOE_3D_SHAPES_*`; run `voe_3d_shape_system_run` each frame
    before the passes, beside the scene systems. In `view.c` and `view.h`, remove the editor's
    sun: `voe_editor_view_pass_camera(view, light)` takes a `voe_render_light`. `main.c` builds
    it from the world's first light row, or zero intensity when the world has none. Rewrite the
    headers of `scene.h`, `view.h` and `main.c` where they describe the four entities, the
    fourth unauthored entity or the editor's sun. Update `editor/editor.md`.
  - Covers: 1 (the scene), 8 (what New makes)
  - Depends on: 8
  - Done when: `cmake -P check.cmake` exits 0 and `d=$(mktemp -d) && ./build/debug/editor/voe_editor --capture $d/shot.png && test -s $d/shot.png` exits 0

- [ ] 11. `editor/` — Projects: open, save, the last project and the command line
  - Change: Add `src/notice.h` and `src/notice.c`:
    `typedef struct { char text[512]; } voe_editor_notice;`,
    `void voe_editor_notice_clear(voe_editor_notice *notice);`,
    `[[gnu::format(printf, 2, 3)]] void voe_editor_notice_set(voe_editor_notice *notice, const char *format, ...);`,
    and `void voe_editor_notice_from_report(voe_editor_notice *notice, const char *file);`, which
    writes `<file>: <voe_base_report_error_first()>`, or `<file>: could not be read` when nothing
    is kept.
    Add `src/project.h` and `src/project.c`. `voe_editor_project` holds its own arena (made in
    `_new…`, freed by `_destroy`), the world in it, the `voe_authoring_kept` it was read with, the
    absolute folder path (NULL when untitled) and `bool unsaved`. The struct lives in its own
    arena. The world's registrations and capacities move here from `main.c`: transform, identity,
    light, shape, mesh, material, panel.
    `voe_editor_project *voe_editor_project_new_untitled(void);` builds the scene of task 10.
    `voe_editor_project *voe_editor_project_new_opened(const char *folder, voe_editor_notice *why);`
    makes the folder absolute, reads `<folder>/project.voe3d` and the scene its `scene` names
    through `platform/file.h`, `voe_authoring_project_read` and `voe_authoring_scene_read`. It
    calls `voe_base_report_error_clear()` before each step. On failure it destroys its arena,
    fills `why` naming the file the step was on — `<folder>/project.voe3d` for a missing marker,
    worded as "is not a project" — and returns NULL. A world that runs out of room is the same
    failure.
    `[[nodiscard]] bool voe_editor_project_save(voe_editor_project *project, const char *folder, voe_editor_notice *why);`:
    `folder` is NULL for an opened project. That writes the scene text from
    `voe_authoring_scene_write` (with its kept sections) to the scene file. For an untitled
    project `folder` is required. The save refuses a folder whose `voe_platform_folder_list`
    count is not 0, with "is not empty" in `why` and nothing written. Otherwise it writes
    `main.scene`, then `project.voe3d` from `voe_authoring_project_write`, and sets the folder. A
    success clears `unsaved`. Scratch text comes from a scratch arena made and destroyed in the
    call.
    `const char *voe_editor_project_name(const voe_editor_project *project);` returns
    `voe_platform_path_name` of the folder, or NULL when untitled.
    `void voe_editor_project_destroy(voe_editor_project *project);`
    Add `src/last_project.h` and `src/last_project.c`. `const char *voe_editor_last_project_read(voe_base_arena *arena);`
    returns the one line of `<settings>/voe3d/last_project` without its newline, or NULL when
    there is no settings folder or no file. That is a first start, and it reports nothing.
    `[[nodiscard]] bool voe_editor_last_project_write(const char *folder);` makes `<settings>` and
    `voe3d` if missing, one level each, then writes the path and a newline.
    In `main.c`, the usage becomes `voe_editor [<folder>] [--capture <path> [--size <W>x<H>]]`,
    and one argument not starting with `--` is the folder; a second one is usage. Before the
    device opens: with a folder, `_new_opened` it. On failure print `voe_editor: <why>` to stderr
    and return 1. Without `--capture`, write the last project and print a failure to write it as
    a warning. With no folder, read the last project. When there is one, try `_new_opened`; on
    failure use `_new_untitled` and keep the notice for task 12's bar, printing it to stderr now.
    `--capture` never writes the last project. The loop, the scene panel and the inspector read
    `project->world`. `voe_editor_scene` keeps its world pointer, set from the project. Rewrite
    `main.c`'s argument paragraph. Update `editor/editor.md`, and the root `README.md`'s Run
    section to say how to open a project and where the last one is remembered.
  - Covers: 5, 6, 10, 11, 12
  - Depends on: 1, 2, 3, 4, 7, 10
  - Done when: `cmake -P check.cmake` exits 0 and the plan's four Verification commands for criteria 12, 11 and 1/6 each exit 0

- [ ] 12. `editor/` — The top bar, New, Save, the unsaved mark and refusing once
  - Change: Add `src/session.h` and `src/session.c`. `voe_editor_session` holds the current
    `voe_editor_project *`, a `voe_editor_notice`, and `armed`, a `voe_editor_command`
    (`NONE`, `NEW`, `OPEN`, `SAVE`, `CLOSE`).
    `bool voe_editor_session_do(voe_editor_session *session, voe_editor_scene *scene, voe_editor_command command);`
    returns true only for a `CLOSE` that goes ahead. A command other than `armed` clears the
    notice and disarms. With `unsaved` set, `CLOSE`, `NEW` and `OPEN` are refused the first time:
    the notice says there are unsaved changes and that doing it again discards them, and the
    command is armed. Armed, the same command goes ahead. `NEW` makes `_new_untitled`, destroys
    the old project and clears the selection. `SAVE` on an opened project calls
    `voe_editor_project_save(…, NULL, …)` and puts a failure in the notice. `SAVE` on an untitled
    project, and `OPEN` once allowed, do nothing yet — tasks 13 and 14 fill them.
    `void voe_editor_session_edited(voe_editor_session *session);` marks the project unsaved,
    clears the notice and disarms.
    Add `src/topbar.h` and `src/topbar.c`: a fixed-height row across the top of the root surface
    holding buttons New, Open and Save, then the project's name or `Untitled`, followed by
    ` (unsaved)` when unsaved, then the notice. Buttons are recorded as the Scene panel's rows are,
    and `voe_editor_command voe_editor_topbar_clicks_read(…)` reads them after
    `voe_ui_frame_end`. `interface.c` lays the bar above the dock tree, and the tree gets the rest
    of the height.
    `inspector.h` and `inspector.c` count the replace intents submitted this frame, in a field
    main reads; a non-zero count calls `voe_editor_session_edited`. A view's camera drag calls
    nothing.
    `main.c`: Ctrl+N, Ctrl+O and Ctrl+S on the frame each key goes down with Control held — edge
    compared with last frame, as the Tab toggle in `dev` does — send the same commands as the
    buttons. `opened.closing` sends `CLOSE`; when refused, call
    `voe_platform_window_close_refuse(window)` and carry on. The startup notice from task 11 goes
    into the session's notice. Raise `VOE_EDITOR_INTERFACE_NODES` and `_ELEMENTS` in
    `interface.h` for the bar's three buttons, its name and its notice, and say in that header's
    comment what the new numbers count. Update `editor/editor.md`.
  - Covers: 1 (the bar says untitled), 2, 3, 5 (Save in an opened project), 8, 9 (close and New)
  - Depends on: 6, 11
  - Done when: `cmake -P check.cmake` exits 0 and `d=$(mktemp -d) && XDG_CONFIG_HOME=$d ./build/debug/editor/voe_editor --capture $d/shot.png && test -s $d/shot.png` exits 0

- [ ] 13. `editor/` — The file browser, and Open
  - Change: Add `src/browser.h` and `src/browser.c`. `voe_editor_browser` holds: whether it
    shows, its mode (`OPEN` or `SAVE`), its own arena for the current folder's absolute path and
    listing (rewound on each navigation), the listed rows — folders only, not hidden, each marked
    when `voe_platform_file_exists(<row>/project.voe3d)` — and the recorded nodes. It keeps its
    folder across showings for the session, and first starts at `voe_platform_folder_home`, else
    `voe_platform_path_absolute(".")`. A listing that fails keeps the previous folder and returns
    the failure as a notice.
    It is drawn as an anchored panel over the dock, below the top bar. At the top are the current
    path and an Up button that does nothing at a root. Then a scroll area with one button per
    folder, whose label ends in ` — project` when marked. At the bottom are a confirm button
    (`Open` in OPEN mode) and `Cancel`.
    `voe_editor_browser_result voe_editor_browser_clicks_read(…)` reports what fired: entered
    row, up, confirm with the folder, cancel. Escape cancels.
    In `session.c`, `OPEN` once allowed shows the browser in OPEN mode. Confirm calls
    `voe_editor_project_new_opened` on the browser's folder. On success it replaces the project,
    writes the last project (a failure to write is a notice, not a refusal), hides the browser
    and clears the selection. On failure the notice is `why`, the browser stays and the project
    is untouched. Every browser action clears the notice and disarms, as a command does.
    `main.c` and `interface.c`: while the browser shows, top-bar commands and shortcuts are
    ignored, the dock panels are handed a pointer with `over = false`, and views get no drag.
    `CLOSE` still follows the unsaved rule. Raise `VOE_EDITOR_INTERFACE_NODES`, `_ELEMENTS` and
    `_SCROLLS` in `interface.h` — the browser is a panel of rows with a scroll area of its own, so
    `_SCROLLS` is one more than the dock's leaves — and say in that header's comment what the new
    numbers count. Update `editor/editor.md`.
  - Covers: 7, 9 (Open), 10 (a broken project from Open)
  - Depends on: 12
  - Done when: `cmake -P check.cmake` exits 0 and `d=$(mktemp -d) && XDG_CONFIG_HOME=$d ./build/debug/editor/voe_editor --capture $d/shot.png && test -s $d/shot.png` exits 0

- [ ] 14. `editor/` — The first save: a typed folder name and the project made
  - Change: `browser.c` in SAVE mode adds a name row: a `voe_ui_field` holding the name and a
    `Make folder` button beside it. The confirm button reads `Save here`. The browser keeps the name
    in a `char [VOE_UI_FIELD_CAPACITY + 1]` of its own and writes back what `voe_ui_field_action`
    hands it after `voe_ui_frame_end`, the way the inspector writes back a number box's value. No
    appending, no Backspace and no UTF-8 stepping is written here — that is task 9's widget. It
    calls `voe_ui_field_focus` on the frame the browser opens for a save, so a name can be typed
    without clicking the box first.
    `main.c` reads `voe_platform_input_text` and the went-down-this-frame edges of Backspace and
    Enter and puts them on the dock root beside its pointer, for the reason the pointer is there;
    `interface.c` hands them over with `voe_ui_keyboard_set` beside `voe_ui_pointer_set`. There is
    no window on a capture, so the keyboard is empty there.
    The field's `entered`, or `Make folder`, with a name refuses one that is empty, `.`, `..`,
    starts with `.`, or contains `/` or `\`, with a notice. Otherwise it calls
    `voe_platform_folder_create(join(folder, name))`, and on success clears the name and enters
    the new folder; a failure is a notice from the report.
    In `session.c`, `SAVE` on an untitled project shows the browser in SAVE mode. `Save here`
    calls `voe_editor_project_save(project, folder, &why)`. On success it writes the last
    project and hides the browser, and the bar then shows the folder's name with no unsaved mark.
    On failure — including "is not empty" — the notice is `why`, nothing was written, and the
    browser stays. Raise `VOE_EDITOR_INTERFACE_NODES` and `_ELEMENTS` again for the name row: a
    field is two nodes, and its records are the box, the caret and one per character.
    Update `editor/editor.md`.
  - Covers: 4, 2 (Save), 6 (a first save becomes the last project)
  - Depends on: 9, 13
  - Done when: `cmake -P check.cmake` exits 0 and `d=$(mktemp -d) && XDG_CONFIG_HOME=$d ./build/debug/editor/voe_editor --capture $d/shot.png && test -s $d/shot.png && test ! -e $d/voe3d` exits 0
