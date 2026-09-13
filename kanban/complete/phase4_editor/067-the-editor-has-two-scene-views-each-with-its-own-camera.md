# 067 — the editor has two scene views, each with its own camera

claimed-by: claude-opus-5 (session 01TYrewyPCZyspC4KzToTd3K)
blocked-by: 064, 065, 066
status: review
decision: *A scene view is a camera's picture on a panel, and there can be many* (ADR-0147) points 1, 2, 4, 5 and 6, on the passes and targets of ADR-0148.

## Goal

`voe_editor` opens on three columns — the Scene list, **two scene views stacked**, the
Inspector. Each view draws the scene's cubes from its own camera into its own target and shows
the picture on its panel. A middle-button drag inside a view moves that view's camera and no
other. Dragging a number in the Inspector moves the cube in both views.

## Scope

**0. The editor may name `3d`** (ADR-0151 point 3). Add `3d` to the editor's row in
`cmake/voe.cmake`, update that row's comment — it currently says `3d` is absent and why — and add
`3d` to `editor/CMakeLists.txt`'s `DEPENDS`. If card 070 has already given the row `authoring`,
keep it.

**1. `editor/src/dock.h`, `dock.c`.**
- `VOE_EDITOR_PANEL_SCENE_VIEW` joins the enumeration.
- A leaf gains `uint32_t view` — which scene view it shows; read only when the panel is
  `SCENE_VIEW`. Say so in the node's comment.
- `voe_editor_dock_default()`: Scene list 20 % | (view 0 over view 1, half each) 60 % |
  Inspector 20 %. Fix the comment on "the one line the card's claim rests on".
- The `SCENE_VIEW` case in `voe_editor_panel_draw` lays out one `voe_ui_image` filling the panel,
  whole picture, from that view's texture, and records the panel's rectangle in millimetres for
  the view to read.

**2. `editor/src/view.h`, `view.c` — new.** A scene view:
- its **camera** — a `voe_scene_camera` value plus an orbit focus and distance — **held by the
  editor, never an entity in the world**;
- its target and texture from `voe_render_target_create`, and its size in pixels;
- the panel rectangle the dock walk recorded last frame.
- `VOE_EDITOR_VIEWS 4`, two in use. Initial cameras: view 0 from the front and above, view 1 from
  the side, both looking at the origin.
- Header: why the camera is not an entity (ADR-0125: never authored, never saved), why the
  picture size lags the layout by one frame, and why a view whose leaf is not in the tree is not
  drawn.

**3. The loop, `editor/src/main.c`.** Per frame:
1. open the frame;
2. for each view **whose leaf is in the tree**: its pixel size is its recorded rectangle times
   pixels per millimetre, rounded; if it changed, `voe_render_target_resize`; open a pass onto
   its target with its camera and the editor's light; `voe_3d_draw_system_run`; close the pass;
3. one pass onto the window with a NULL camera for the interface, as card 064 left it;
4. close the frame.
- The camera handed to the pass is built from the view's `voe_scene_camera` with
  `voe_scene_camera_view` and `voe_3d_projection`, aspect from the view's own size — **not**
  `voe_3d_draw_system_frame`, which reads a camera out of the world.
- **The light is the editor's**: one fixed direction and intensity in `view.c`, not an entity.
- Capacities: `targets` at least `VOE_EDITOR_VIEWS`, `passes` at least `VOE_EDITOR_VIEWS + 1`.

**4. The scene, `editor/src/scene.c`.** Register `3d`'s mesh and material components. `Cube` and
`Cube_2` get a cube mesh (layer WORLD) and one shared plain material. `Marker` and the unauthored
entity stay without. The cube geometry is placeholder data at a call site, as the scene is:
`editor/src/cube.c`, copied from `dev/src/cubes.c` with its winding comment — a leaf cannot
include another leaf. Place the cubes apart so both are visible from both initial cameras.
**The Scene list still shows three rows.** If the mesh or material components carry field
descriptions the Inspector will show them; do not add or remove descriptions.

**5. Camera controls, in `view.c`, read in `main.c` beside the pointer.**
- A middle-button press **that starts over a view's panel** captures that view until release;
  the drag moves only that view, even when the pointer leaves its panel.
- Middle-drag: orbit about the focus. Shift + middle-drag: pan the focus. Control + middle-drag:
  dolly the distance, clamped above zero.
- The left button and the interface behave exactly as before; a left click in a view does nothing.
- Rates are constants at the top of `view.c`, in radians or metres per millimetre of pointer travel.

**6. `editor/editor.md`** — the files list gains `view.h`/`view.c` and `cube.c`; the absent list
loses "no viewport, no camera, no light and no 3D" and says what remains absent.

## What must not change

- `render`, `ui`, `3d`, `scene`, `platform` — beyond the one edge in point 0. A gap found in any of them is a report, not an
  edit here.
- The Scene list's three rows, the selection, the Inspector's drag, replace intents.
- The one pixels-to-millimetres division stays in `main.c`.
- No picking, no gizmo, no grid, no view modes, no view header bar, no splitter, no wheel.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest` passes.
- Run `./build/debug/editor/voe_editor`. In Notes, say that you saw: both cubes in both views
  from different angles; orbit, pan and dolly in view 0 leaving view 1 still; the same in view 1;
  a drag started in a view continuing past its edge; a transform dragged in the Inspector moving
  the cube in both views; the window resized and both pictures still sharp and undistorted.
- Take a screenshot of the editor with the two views at different angles and put its path in
  Notes.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

Two scene views of the same world from two cameras, each moved on its own, both updating live as
the Inspector edits a cube.

## Notes

**Verified on Linux only** (Fedora 44, KDE Wayland, clang 22.1.8, RTX 4070). Windows not checked.
- `cmake -P check.cmake` exits 0: 45 tests, analyser clean, `editor` standalone. `tools/hot.sh`: no `OVER`.
- `voe_editor` on the GPU, driven by a throwaway virtual mouse (uinput) and KWin scripts, a screenshot
  per step, each view's half compared pixel by pixel. Seen: both cubes in both views from different
  angles; orbit, pan and dolly in view 0 changed view 0 and left view 1 identical to the pixel; the
  same the other way round in view 1; a drag started in view 0 still turned view 0 while held over
  view 1 and then over the Scene column, with view 1 unchanged; a left click in a view changed
  nothing; `Cube` selected and its position x dragged −1.250 → −1.038 moved the cube in both views;
  window resized to 1600×640 and 800×900 with both pictures sharp and the cubes not stretched.
- Screenshot: `067-two-scene-views.png` beside this card (views moved, Inspector drag done).

**Readings taken, none marked in code because none departs from the card:**
- Registration is in `main.c` beside transform and identity, not in `scene.c`: `scene.h` already
  says the world arrives registered. `voe_3d_panel_register` is added too: the draw system walks
  panels and asserts on an unregistered table (`3d/draw_system.h`).
- `cube.h` added beside `cube.c` for the declarations.
- `voe_editor_scene_build` now takes the device and returns `bool`: uploads can be refused (rule 13).
- A scene view's panel has no padding, so the picture's rectangle is the panel's.
- Resizes are asked for before `voe_app_draw_open`, so they apply in the same frame and the aspect
  matches the target.
- `VOE_EDITOR_VIEWS_IN_USE 2` names the "two in use".

**Found, not fixed (not this card's):**
- At 20 % the Inspector is narrower than the transform's three number boxes, so they run off its
  right edge (visible in the screenshot). That comes from the layout this card sets plus "no
  clipping, no scrolling"; it is a layout decision.
- The seam between the two views is invisible: both targets clear to the window's colour.
- `editor.md` has headed sections, which `guidelines.md` forbids. It was like that before this card.
- Harness note: a uinput mouse with no wheel gets its middle button taken by libinput for button
  scrolling, so it never reaches the window. Give a virtual mouse `REL_WHEEL`. Not an engine fault.
