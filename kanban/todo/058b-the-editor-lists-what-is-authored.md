# 058b — A scene built in code, and the Scene panel lists what is authored

claimed-by: -
blocked-by: 055, 058a
status: todo
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
