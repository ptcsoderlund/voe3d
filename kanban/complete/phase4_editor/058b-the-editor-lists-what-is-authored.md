# 058b — A scene built in code, and the Scene panel lists what is authored

claimed-by: kanban-coder (Opus 5)
blocked-by: 055, 058a
status: review
decision: *Identity … its presence means authored* (ADR-0125) — the list is exactly the entities with an identity; *An edit is a replace intent* (ADR-0134) point 7 — the owning systems run every frame; *The editor's panel layout is a tree of data* (ADR-0142) point 2 — a panel's contents are ordinary C and the tree owns only geometry.

## Goal

The `Scene` panel lists the names of the authored entities and clicking one selects it; the
`Inspector` panel says which is selected. Card 059 puts the real inspector there.

## Scope

**1. `editor/src/scene.h`, `editor/src/scene.c` — the scene the editor opens on, built in
code** until there is a loader: `Cube` (id 1), `Cube_2` (id 2, turned) and `Marker` (id 3,
scale 0.5), each with an identity and a transform, and **one entity with a transform and no
identity**, which must not appear in the list.

**2. `editor/src/main.c`** gains `voe_scene_transform_register`, `voe_scene_identity_register`
and the scene, and runs `voe_scene_transform_system_run` and `voe_scene_identity_system_run`
every frame inside the loop.

**3. Selection.** An entity id the editor holds, beside the roots. Clicking selects it; an
entity that no longer exists is no selection. The header that declares it says selection
belongs to the editor and not to the dock tree — **a panel reads it, the tree does not know it
exists.**

**4. The two panels, in `voe_editor_panel_draw`.**

- `SCENE`: the heading `Scene`, then one button per row of the identity table
  (`voe_scene_identity_rows` and `_entities`), labelled with the name and keyed by the name
  `entity` and the row index. The selected one's label starts with `> `.
- `INSPECTOR`: `Selected: <name>` or `Nothing selected`.

**5. `editor/editor.md`** — the two new files.

## What must not change

- **No folder among `base`, `math`, `ecs`, `platform`, `scene`, `assets`, `render`, `3d`,
  `text`, `ui`, `sprite` or `app` gains anything for the editor** (ADR-0121 point 6). A gap is
  `BLOCKED: <folder>, <why>`.
- **The dock tree and its walk are not edited.** This card adds panel contents only; if a
  panel's contents cannot be drawn without changing the tree, that is a finding worth a line in
  Notes rather than a change made quietly.
- Nothing becomes rearrangeable: no splitter, no tabs, no dragging, no saved layout.
- No viewport, no 3D, no camera, no light, no save, no load, no create or delete.
- `dev` is not edited.

## Verify

- Linux: `cmake -P check.cmake` green.
- `cmake --preset editor`; build and run `voe_editor`: three names in the left panel, not four;
  clicking one selects it and the right panel follows.
- The fourth, unnamed entity is confirmed present in the world and absent from the list — say
  in Notes how that was checked.
- `grep -rln 'editor/' base math ecs platform scene assets render 3d text ui sprite app`
  returns nothing.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

`Cube`, `Cube_2` and `Marker` in the left panel, one of them selected by a click — and a
fourth entity with no name that is correctly not there.

## Notes

Verified on **Linux** (Fedora, Wayland, NVIDIA RTX 4070 Laptop, Vulkan 1.4.341,
Clang 20). Windows unchecked — nothing in this card is platform-aware, and the
whole of it is `editor/`.

- `cmake -P check.cmake` **exits zero**: every folder standalone (`editor`
  included), root configure and build, the three guards, includes, 42 tests, and
  the analyser over 117 files.
- `cmake --preset editor`, built and ran `voe_editor`: **three names in the left
  panel and not four** — `Cube`, `Cube_2`, `Marker` — and `Nothing selected` on
  the right.
- **Clicking.** This machine has no way to inject a pointer event into a Wayland
  window — no `/dev/uinput`, no `ydotool`, and X tools do not reach it. So the
  click was driven by a **temporary probe in `main.c`, reverted before this card
  moved**: a synthetic pointer swept down the left panel pressing and releasing
  at each millimetre. It reported `y=10 mm selected Cube`, `y=22 mm selected
  Cube_2`, `y=34 mm selected Marker` — all three rows, each to its own entity.
  A second probe clicked the first row and stopped, and the window then showed
  **`> Cube`** in the list and **`Selected: Cube`** in the inspector.
- **The fourth entity, both halves of the claim.**
  `voe_editor_scene_build` asserts `voe_scene_transform_count(world) == 4` and
  `voe_scene_identity_count(world) == 3` at the end of the build, so the fourth
  being *present* is checked in the program on every debug run rather than by a
  person counting; the run above is that assert passing. Its *absence* from the
  list is the screen: three buttons, and the list walks
  `voe_scene_identity_rows`/`_entities` and has nothing else it could walk.
- `grep -rln 'editor/' base math ecs platform scene assets render 3d text ui
  sprite app` over sources is **empty**. Bare, it matches only each folder's
  generated `compile_commands.json`, which `.gitignore` line 22 ignores.
- `bash tools/hot.sh` at the planning root: **all hot files under their
  ceilings**, no `OVER`. `editor/editor.md` is 96 / 120.

### The finding this card asked for a line about

**`voe_editor_panel_draw` needed one more parameter, so the walk's two signatures
gained one too.** The card puts the panels' contents in
`voe_editor_panel_draw`, which lives in `dock.c` and is called from inside
`walk_node`; the contents read the identity table and the selection, and neither
can reach that function without being handed in. So `voe_editor_dock_walk`,
`walk_node` and `voe_editor_interface_draw` each carry a `voe_editor_scene *`
straight through.

**The tree itself is untouched** — no field on `voe_editor_dock_node`, none on
`voe_editor_dock_tree`, none on `voe_editor_dock_root`, and no change to the
axes, the fractions, the seam or the geometry. A tree built for one scene lays
out identically for another, and `dock.h` says so where the parameter is
declared. The alternative to the parameter was this folder reaching for a
global.

### Two more, neither touched

- **Reading the click has to happen inside `voe_editor_interface_draw`.** A `ui`
  widget answers what the pointer did only between `voe_ui_frame_end` and the
  rewind of the arena its nodes came out of, and both of those are that
  function's. `interface.c`'s header said it decides nothing about what is on a
  panel; it now also says why the one question it *does* ask is asked there, and
  what a click *means* is still `scene.c`'s.
- **`editor/editor.md` has `##` sections, which `guidelines.md` says a
  `<folder>.md` may not have** — the `# <folder>` title is meant to be the only
  heading. It is the only one of the twelve that does; the others have none. It
  arrived that way with card 058a, so it is reported rather than restructured on
  the way past.

### A suggestion, not in the diff

The selected row is told apart by a `> ` label because `ui` has no theme yet and
a button has one look in each of three states. When card 036's theme arrives, a
selected row wants to be a **colour** and the marker should go.
