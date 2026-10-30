# 04 — The folds a person left, kept in editor_settings
folder: editor
after: 03
decisions: 0168, 0502, 0503

## Change
Make the fold store the Inspector draws from. Nothing draws a section yet.

- New `editor/src/folds.h` and `folds.c`. `voe_editor_folds` holds:
  - the folded names: `VOE_EDITOR_FOLDS` (128) of them, each at most `VOE_EDITOR_FOLD_NAME` bytes,
    which is room for `kind 15 ` and a `VOE_3D_MODEL_PATH` path;
  - this frame's fold buttons, `VOE_EDITOR_FOLDS_SHOWN` (64), each a node and a name copied into
    the frame arena. That keeps the struct small, and the scene sits on main's stack.

  Functions:
  - `void voe_editor_folds_load(voe_editor_folds *folds)`: empty the store, then take every
    `fold <name>` line through settings.h's `voe_editor_settings_lines_read`. A line that does not
    fit is dropped and nothing is reported.
  - `bool voe_editor_folds_open(const voe_editor_folds *folds, const char *name)`: no line means
    open.
  - `void voe_editor_folds_forget(voe_editor_folds *folds, voe_base_arena *frame)`: empty this
    frame's buttons and take the frame's arena. Called at frame open.
  - `bool voe_editor_folds_begin(voe_ui_context *ui, voe_editor_folds *folds, const char *name, const char *key, uint32_t index)`:
    call `voe_ui_section_begin` keyed `key` and `index`, open as `name`'s line says, and record its
    fold button under `name`. Return whether it is open. The header row is left open for the
    caller's title.
  - `void voe_editor_folds_read(const voe_ui_context *ui, voe_editor_folds *folds)`: after
    `voe_ui_frame_end`, a fired fold adds or drops its name. On any change, write the file once.
  - `void voe_editor_folds_clear(voe_editor_folds *folds, const char *name)`: drop the name, and
    write the file only if the name was there.
  - `void voe_editor_folds_section_name(char *out, size_t room, const char *word)` builds
    `section <word>`. `void voe_editor_folds_kind_name(char *out, size_t room, const char *landscape, uint32_t kind)`
    builds `kind <k> <path>`.

  The write goes through `voe_editor_settings_lines_write`. It owns every line starting `fold `,
  writes one `fold <name>` line per folded name, and reports a failure at the site.

  The header makes these points: names as 0502 and 0503 point 5 say; only folded things are
  written; the file is written when a fold flips; folds are the person's, never undone and never
  in a project.
- `editor/src/settings.h`: the "own, not for other callers" note also names `folds.c`. The line
  list says the `fold` lines are folds.h's.
- `editor/src/scene.h`: add `voe_editor_folds folds;` to `voe_editor_scene` beside `snap`. It is
  the person's, read at start by main.c.
- `editor/src/main.c`: call `voe_editor_folds_load(&scene.folds)` beside the snap read (about
  line 402).
- `editor/src/interface.c`: call `voe_editor_folds_forget(&scene->folds, arena)` where
  `voe_editor_inspector_frame_begin` is called (about line 284).
- `editor/src/interface_read.c`: call `voe_editor_folds_read(ui, &scene->folds)` beside
  `voe_editor_inspector_sculpt_read` (about line 148).
- `editor/src/src.md`: add `folds.h` and `folds.c` entries. `settings.h`'s entry names the fold
  lines.

## Done when
`grep -q voe_editor_folds_load editor/src/main.c && grep -q voe_editor_folds_forget editor/src/interface.c && grep -q voe_editor_folds_read editor/src/interface_read.c`
exits 0.
