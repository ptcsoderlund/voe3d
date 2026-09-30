# 07 — The transform is removable when nothing needs it
folder: editor
after: 06
decisions: 0168, 0300, 0302

## Change
0302 point 4. Read `editor/src/inspector.h`, `editor/src/inspector.c` (the
walk over types near line 700 and `component_panel`), `editor/src/dock.c`
(`inspector_panel` near line 645) and `editor/src/src.md`.

- `editor/src/dock.c`: the kept list holds the identity and the camera, not
  the transform; the comment above `inspector_panel` says why the transform
  left (0300: it is removable when nothing needs it).
- `editor/src/inspector.c`: in the walk, a type is also not removable while
  another row the selected entity holds names it as needed
  (`voe_ecs_component_needs`), found by a small static function walking the
  world's types; a runtime-only row counts too, since it still needs the
  place. The section of such a type shows no Remove, the same as a kept one.
  The Inspector still names no component.
- `editor/src/inspector.h`: the "WHICH ROWS AN ENTITY HOLDS" paragraph:
  kept types are the identity and the camera now; a type has no Remove
  while another row of the entity needs it (0302), which is how the
  transform stays while a shape is there.
- `editor/src/src.md`: the `inspector.c` entry, if it names the kept
  types.

## Done when
`grep -c voe_scene_transform_key editor/src/dock.c` prints 0, and the editor
builds in the folder's checks.
