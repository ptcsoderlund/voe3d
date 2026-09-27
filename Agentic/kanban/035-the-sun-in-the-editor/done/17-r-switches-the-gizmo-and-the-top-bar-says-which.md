# 17 — R switches the gizmo, and the top bar says which
folder: editor
decisions: 0168, 0274

## Change
Needs cards 13 and 16 (16 adds `VOE_PLATFORM_KEY_R`).

- `editor/src/shortcuts.h`, `editor/src/shortcuts.c`: a `gizmo_switch` flag, true on R's down
  edge (`VOE_PLATFORM_KEY_R`, no modifier) under the same quiet guard Delete uses (not typing,
  not flying, no browser, picker or dropdown). The header's list of keys names it.
- `editor/src/main.c`: where the shortcut flags are acted on, call
  `voe_editor_scene_gizmo_switch(&scene)` when the flag is set. One or two lines: the file is
  near 800.
- `editor/src/topbar.h`, `editor/src/topbar.c`: `voe_editor_topbar_draw` gains
  `const char *gizmo`, drawn as a label (not a button) after Preferences and before the
  project's name; the header's description of the row says so.
- `editor/src/interface.c`: its one call of `voe_editor_topbar_draw` passes `"Rotate"` when
  `scene->rings`, else `"Move"`.
- `editor/src/src.md`: the `shortcuts` and `topbar` lines.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and
`build/debug/editor/voe_editor examples/coin_game --capture <scratch>/bar.png` exits 0 with a
PNG in which the top bar reads Move (look at it once).
