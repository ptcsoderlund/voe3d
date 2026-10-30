# 05 — Every Inspector section folds under a heading
folder: editor
after: 04
decisions: 0168, 0496, 0502, 0503

## Change
Every section opens with `voe_editor_folds_begin` (folds.h). Its title is a
`VOE_UI_TEXT_ROLE_HEADING` label, and its body is drawn only while open.

- `editor/src/inspector.h`: `voe_editor_inspector_draw` takes `voe_editor_folds *folds` after
  `sculpt`. The header says: every section folds by its fold button, named as 0502 says; a title's
  click does nothing.
- `editor/src/inspector.c`, each described component's section (the identity's too):
  - its name is `voe_editor_folds_section_name` of its component key (`voe_ecs_component_key`);
  - it is keyed by its type and titled with its heading;
  - Remove moves onto the header line, after the title;
  - the "Needs" line and the fields are its body.

  Enabled, Duplicate and Delete, a part's line, Add component and the open material's section stay
  as they are. Pass `folds` on to the five draws below.
- `editor/src/inspector_sculpt.h` and `.c`, `inspector_layers.h` and `.c`, `inspector_foliage.h`
  and `.c`, `inspector_brush.h` and `.c`, `inspector_level.h` and `.c`: each draw takes
  `voe_editor_folds *folds`. Its title label becomes a section named by its word: `sculpt`,
  `layers`, `foliage`, `brush`, `lod`. The rest of the section is its body. Foliage's kind rows
  stay as they are inside its body; 06 boxes them. Each header says the section folds.
- `editor/src/dock_walk.c`: pass `&scene->folds` at the `voe_editor_inspector_draw` call (about
  line 515).
- `editor/src/interface.h`: each section adds four nodes and five element records. If the frame's
  budget is too small for that, raise it here, with a comment saying why.
- `editor/src/src.md`: the changed files' entries say their section folds.

## Done when
`test "$(cat editor/src/inspector.c editor/src/inspector_sculpt.c editor/src/inspector_layers.c editor/src/inspector_foliage.c editor/src/inspector_brush.c editor/src/inspector_level.c | grep -c voe_editor_folds_begin)" -ge 6`
exits 0.
