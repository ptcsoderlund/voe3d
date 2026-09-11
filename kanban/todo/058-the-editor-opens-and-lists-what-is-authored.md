# 058 — The editor opens: a window, a scene built in code, and the list of what is authored

claimed-by: -
blocked-by: 051, 055
status: todo
decision: *The editor is a module in this repository* (ADR-0121) — a leaf program no folder depends on; *`app` is parts* (ADR-0135) for its loop; *Identity … its presence means authored* (ADR-0125) — the list is exactly the entities with an identity; *An edit is a replace intent* (ADR-0134) point 7 — the owning systems run every frame.

## Goal

`voe_editor` opens a window, holds a world with a few authored entities, and lists their
names; clicking one selects it. No inspector yet — card 059.

## Scope

**1. Wiring.**

- `cmake/voe.cmake`, `voe_allowed_deps`: an `editor` row, `base math ecs scene platform
  render text ui app`, commented as a leaf like `dev` that appears in no other row.
- Root `CMakeLists.txt`: `add_subdirectory(editor)` after `dev`.
- `editor/CMakeLists.txt`, the four lines:
  `voe_executable(editor DEPENDS app ui text scene ecs render platform math base)`.
- `CMakePresets.json`: a configure and a build preset `editor` — `debug`'s settings, its own
  binary directory, and `-DVOE_BASE_DESCRIPTIONS=1` in `CMAKE_C_FLAGS` (appended if `debug`
  sets that variable). `debug` and `release` keep descriptions off.
- `editor/editor.md`, the folder page, including how to build with the `editor` preset.

**2. `editor/src/scene.h`, `editor/src/scene.c` — the scene the editor opens on, built in
code** until there is a loader: `Cube` (id 1), `Cube_2` (id 2, turned) and `Marker` (id 3,
scale 0.5), each with an identity and a transform, and **one entity with a transform and no
identity**, which must not appear in the list.

**3. `editor/src/main.c`.**

- `voe_app_new`: 1280×720, titled `voe3d editor`, capacities enough for the interface's
  element records and small elsewhere — read `render/include/render/device.h` on what a
  capacity of zero means. No present-mode request.
- An arena, `voe_ecs_world_new`, `voe_scene_transform_register`,
  `voe_scene_identity_register`, the scene, `voe_text_font_new`, a `ui` context.
- At startup, one line on stdout: whether descriptions are compiled in, and if not, that
  nothing will be expandable and that the `editor` preset turns them on.
- The loop: `voe_app_frame_open`, `break` on `closing`; `voe_scene_transform_system_run` and
  `voe_scene_identity_system_run` every frame; `voe_app_draw_open` with a zeroed
  `voe_render_view` and `voe_render_light` — the editor draws no world, and `render` does not
  check those values; when `drawing`, the interface; then `voe_app_draw_close`.

**4. `editor/src/interface.h`, `editor/src/interface.c` — on the screen-filling surface, the
way `dev/src/interface.c` does it.** Pixels per millimetre from the window's height over a
135 mm surface; `voe_render_element_surface_size`; the pointer's pixels divided into
millimetres; `.fine` from `VOE_PLATFORM_KEY_SHIFT`; the records submitted, then drawn with
`voe_render_element_transform`. Copy the shape; do not include `dev`'s files.

- **The scene list**: a panel down the left headed `Scene`, one button per row of the identity
  table (`voe_scene_identity_rows` and `_entities`), labelled with the name and keyed by the
  name `entity` and the row index. The selected one's label starts with `> `.
- **Selection**: an entity id the editor holds. Clicking selects; an entity that no longer
  exists is no selection.
- On the right, a heading `Selected: <name>` or `Nothing selected` — where card 059 puts the
  inspector.

## What must not change

- **No folder among `base`, `math`, `ecs`, `platform`, `scene`, `assets`, `render`, `3d`,
  `text`, `ui`, `sprite` or `app` gains anything for the editor** (ADR-0121 point 6). A gap is
  `BLOCKED: <folder>, <why>`.
- No viewport, no 3D, no camera, no light, no save, no load, no create or delete.
- `dev` is not edited.

## Verify

- Linux: `cmake -P check.cmake` green — it builds `voe_editor` with descriptions off.
- `cmake --preset editor` and a build of `voe_editor` succeed.
- Run it: three names, not four; clicking selects and the heading follows; the startup line
  is right in both builds.
- `grep -rln 'editor/' base math ecs platform scene assets render 3d text ui sprite app`
  returns nothing.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A window listing `Cube`, `Cube_2` and `Marker`, where clicking one says it is selected — and
a fourth entity with no name that is correctly not there.

## Notes
