# 05 — F is a shortcut flag
folder: editor/src
after: 01, 04
decisions: 0168, 0371

## Change
F's down edge becomes a shortcut flag, decided once where every other key's meaning is
(decision 0371 point 4). Nothing acts on it yet.

- `editor/src/shortcuts.h` — `voe_editor_shortcuts` gains `bool frame_selection;` with a comment
  beside the gizmo switch's: F alone, the view under the pointer frames the selection (view.h).
  The header's opening list of which edge means which command names F.
- `editor/src/shortcuts.c` — set it from `keys->pressed[VOE_PLATFORM_KEY_F]` under exactly the
  conditions `gizmo_switch` is set under (no Control, and the same guards), so typing, a flying
  view, the browser, the picker and an open list silence it.
- `editor/src/src.md` — `shortcuts.c`'s entry lists F beside R.

## Done when
- `cmake --build --preset debug --target voe_editor` exits 0.
- `grep -n "VOE_PLATFORM_KEY_F" editor/src/shortcuts.c` finds the read.
