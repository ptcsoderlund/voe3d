# 058a — The editor opens: a window, and a panel layout that comes from data

claimed-by: -
blocked-by: -
status: todo
decision: *The editor is a module in this repository* (ADR-0121) — a leaf program no folder depends on; *`app` is parts* (ADR-0135) for its loop; *The editor's interface is world geometry and its camera is replaceable* (ADR-0141) points 3 and 4 — no snapping, and the interface is handed its pointer in panel millimetres; *The editor's panel layout is a tree of data* (ADR-0142) points 1 to 6 — what the tree is, what it is not, and what is deliberately absent.

## Goal

`voe_editor` opens a window and draws two named regions side by side, **whose rectangles came
from a tree of data rather than from the order of the calls**. Nothing is draggable and no
scene exists yet — card 058b brings the scene and the list.

## Scope

**1. Wiring.**

- `cmake/voe.cmake`, `voe_allowed_deps`: an `editor` row, `base math ecs scene platform
  render text ui app`, commented as a leaf like `dev` that appears in no other row.
- Root `CMakeLists.txt`: `add_subdirectory(editor)` after `dev`.
- `editor/CMakeLists.txt`:
  `voe_executable(editor DEPENDS app ui text scene ecs render platform math base)`.
- `CMakePresets.json`: a configure and a build preset `editor` — `debug`'s settings, its own
  binary directory, and `-DVOE_BASE_DESCRIPTIONS=1` in `CMAKE_C_FLAGS` (appended if `debug`
  sets that variable). `debug` and `release` keep descriptions off.
- `editor/editor.md`, the folder page, including how to build with the `editor` preset, and
  **one line saying why the layout is data and not call order, naming ADR-0142** — a reader
  who arrives before anything is draggable will otherwise read the tree as indirection for
  its own sake.

**2. `editor/src/dock.h`, `editor/src/dock.c` — the tree.**

- A panel is an enumeration in this header: `VOE_EDITOR_PANEL_SCENE`,
  `VOE_EDITOR_PANEL_INSPECTOR`, and a count. **No panel is a string and none is registered.**
- A node is one of two kinds in a tagged struct: a **split** — an axis (`ROW` or `COLUMN`), a
  `double fraction`, and the two children as indices into the node array — or a **leaf**
  naming one panel. Nodes live in a fixed array in the tree; the tree holds the root index.
- A root is a struct holding a tree, the surface size in millimetres, and the pointer. **The
  editor holds an array of roots with one entry**, and the loop walks every root. The header
  says root zero is the window, that a detached window is a further root, and — pointedly —
  that this does not make a second OS window cheap, because `platform`, `render` and `app`
  each assume one.
- `voe_editor_dock_walk(root, ui, ...)`: a recursive walk. A split emits a `ui` row or column;
  **each child is given a fixed size along the axis, computed from the parent's content size
  in millimetres and the fraction**, and fills the cross axis. A leaf emits a `ui` panel and
  calls `voe_editor_panel_draw` for its panel. Depth is asserted against
  `VOE_EDITOR_DOCK_DEPTH` (16) — a dock tree is shallow, and a cycle in the indices is a
  program's bad value.
- **No function pointer anywhere in this folder.** `voe_editor_panel_draw` is one function
  with a `switch` over the enumeration, the way `app` keeps callbacks out.
- `voe_editor_dock_default()`: the tree this card opens on — a `ROW` split at 0.25, the left
  leaf `SCENE`, the right leaf `INSPECTOR`.
- The header says the fraction is the thing a splitter will later write back, and that nothing
  writes it today.

**3. `editor/src/main.c`.**

- `voe_app_new`: 1280×720, titled `voe3d editor`, capacities enough for the interface's
  element records and small elsewhere — read `render/include/render/device.h` on what a
  capacity of zero means. No present-mode request.
- An arena, `voe_ecs_world_new`, `voe_text_font_new`, a `ui` context, and the default tree.
- At startup, one line on stdout: whether descriptions are compiled in, and if not, that
  nothing will be expandable and that the `editor` preset turns them on.
- The loop: `voe_app_frame_open`, `break` on `closing`; `voe_app_draw_open` with a zeroed
  `voe_render_view` and `voe_render_light` — the editor draws no world, and `render` does not
  check those values; when `drawing`, the interface; then `voe_app_draw_close`.
- **`main.c` converts the window's pointer into the root's millimetres and hands it in.** That
  division lives here and nowhere else (ADR-0141 point 4).

**4. `editor/src/interface.h`, `editor/src/interface.c` — on the screen-filling surface, the
way `dev/src/interface.c` does it.** Pixels per millimetre from the window's height over a
135 mm surface; `voe_render_element_surface_size`; `.fine` from `VOE_PLATFORM_KEY_SHIFT`; the
records submitted, then drawn with `voe_render_element_transform`. Copy the shape; do not
include `dev`'s files.

- **It takes the pointer as a parameter, already in millimetres**, and computes no pointer of
  its own. Its header says why, in one line, naming ADR-0141.
- It begins the frame, walks each root, ends the frame, submits.
- `voe_editor_panel_draw` this card: `SCENE` draws a heading `Scene`, `INSPECTOR` draws
  `Nothing selected`. That is all a panel does yet.

## What must not change

- **No folder among `base`, `math`, `ecs`, `platform`, `scene`, `assets`, `render`, `3d`,
  `text`, `ui`, `sprite` or `app` gains anything for the editor** (ADR-0121 point 6). A gap is
  `BLOCKED: <folder>, <why>`.
- **Nothing is rearrangeable**: no splitter drag, no tab bar, no dragging a panel, no closing
  one, no scrolling, no clipping, no saved layout, no second root (ADR-0142 point 6).
- No viewport, no 3D, no camera, no light, no pixel snapping anywhere.
- `dev` is not edited.

## Verify

- Linux: `cmake -P check.cmake` green — it builds `voe_editor` with descriptions off.
- `cmake --preset editor` and a build of `voe_editor` succeed.
- Run it: two regions side by side, the left a quarter of the width, headed `Scene` and
  `Nothing selected`. Resize the window: the split stays a quarter.
- Change `voe_editor_dock_default` to a `COLUMN` at 0.5, rebuild, and the two regions are
  stacked — **say in Notes that this was tried and then reverted.** It is the only proof this
  card's point was met.
- `grep -rln 'editor/' base math ecs platform scene assets render 3d text ui sprite app`
  returns nothing.
- `grep -rn '(\*[a-z_]*)(' editor/src` finds no function pointer.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A window split into two named regions, where changing one number in one function stacks them
instead — and a tree nobody can drag yet, on purpose.

## Notes
