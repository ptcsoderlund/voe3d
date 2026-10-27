# 06 — Snap is remembered in editor settings
folder: editor
after: 05
decisions: 0168, 0482, 0438, 0220, 0226

## Change
0482 point 5: the snap state, its read at start and its write, and where it lives. Nothing shows
or uses it yet (cards 07, 08).

- `editor/src/settings.h` — 
  - `typedef struct { bool on; float move; float rotate; float scale; } voe_editor_snap;`
    move in metres, rotate in degrees;
  - `VOE_EDITOR_SNAP_MOVE` 0.5, `VOE_EDITOR_SNAP_ROTATE` 15, `VOE_EDITOR_SNAP_SCALE` 0.1, and
    each step's least and most (0482 point 5);
  - `voe_editor_snap voe_editor_settings_snap_read(void)` — off with the defaults, each field
    the file has a parsing, in-range line for read over them;
  - `[[nodiscard]] bool voe_editor_settings_snap_write(const voe_editor_snap *snap)` — re-reads
    the file, keeps every other key's line in its order, writes `snap`, `snap_move`,
    `snap_rotate`, `snap_scale` after them.
  Header: the four keys and their forms; the panel write already keeps them; the steps are the
  person's, not the project's (0438).
- `editor/src/settings.c` — the two, sharing the existing line read and keep-other-keys write
  rather than a second copy of it.
- `editor/src/scene.h` — field `voe_editor_snap snap` beside `gizmo_kind`, the person's; the
  mode paragraph says so.
- `editor/src/main.c` — after the scene is zeroed, `scene.snap = voe_editor_settings_snap_read()`.
- `editor/src/src.md` — the `settings` entries name the snap keys.

## Done when
The editor builds, and `grep -n "snap_move" editor/src/settings.c` prints a line.
