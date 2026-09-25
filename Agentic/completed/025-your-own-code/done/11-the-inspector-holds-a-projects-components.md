# 11 — The Inspector and Add component hold a project's components, named as people read them
folder: editor
decisions: 0168, 0241, 0242

## Change
Point 9 of 0242, first half, and the room for point 3's 32 more types. Read the headers of
`editor/src/inspector.h`, `editor/src/add_menu.h`, `editor/src/interface.h` and
`game/world.h` / `game/project.h`, and the static `heading()` in `editor/src/inspector.c`.

- `editor/src/inspector.c`, `editor/src/inspector.h` — `heading()`: a key name that starts
  `voe_<folder>_` drops that; every remaining `_`-separated word is capitalised and joined by a
  space (`voe_scene_transform` → "Transform", `keyboard_input` → "Keyboard Input",
  `follow_camera` → "Follow Camera"). `VOE_EDITOR_INSPECTOR_SECTIONS` becomes
  `VOE_GAME_WORLD_TYPES + VOE_GAME_PROJECT_TYPES`, its comment naming both. The header says how a
  heading is made. If an engine heading changes by the new rule, say which in the card's commit.
- `editor/src/add_menu.h` — `VOE_EDITOR_ADD_MENU_ENTRIES` covers every engine and project type
  plus the folder entries their menu paths open; its comment says so.
- `editor/src/interface.h` — the paragraph that says "never more than eight component types" and
  the NODES and ELEMENTS sums sized from the section count, redone for the new count; the numbers
  raised to match.
- `editor/src/src.md` — nothing unless an entry's sentence changes.

## Done when
1. `checks.sh --folder editor` prints `FINDINGS: 0`.
2. In `p=$(mktemp -d)` with `cp -r game/example/. $p`, `build/debug/editor/voe_editor --capture
   $p/shot.png $p` exits 0 and its stderr is empty.
3. `grep -n 'VOE_EDITOR_INSPECTOR_SECTIONS' editor/src/inspector.h` shows it defined from the two
   game constants.
