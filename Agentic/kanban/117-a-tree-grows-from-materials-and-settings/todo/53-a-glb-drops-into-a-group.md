# 53 — A glb drops into a group
folder: editor
after: 52
decisions: 0168, 0520, 0521, 0442

## Change
`editor/src/assets_drag.h`, `assets_drag.c`, and the drop rects in `editor/src/inspector_foliage.c`:
- a glb dropped on a group's title is added to it (`voe_editor_models_kind_set`'s successor with
  glb = `glb_count`); refused at `VOE_ASSETS_LANDSCAPE_GROUP_GLBS` or when the group holds that path;
- a glb dropped on a glb's title replaces its path, keeping its look; refused when another glb of the
  group holds it;
- a glb dropped on the add row makes a group; a seventeenth is refused as a seventeenth kind is today;
- the drag's ghost shows a refused drop as it shows any refused drop.

None of these is an undo step; each marks the landscape unsaved, as a kind's drop does (0521 point 5).
The new glb's box shows open.

Header points: the three drop targets and their refusals. Entries in `editor/src/src.md`.

## Meaning
- Scout: a drop on a kind row replaces the path keeping its look; the add row adds a kind with a new seed,
  refused at 16, with no undo step. Nothing stops a path twice.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -c 'GROUP_GLBS' editor/src/assets_drag.c` prints 1 or more.
