# 15 — editor/src headers and entries fit the rules
folder: editor/src
after: none
decisions: 0168, 0402

## Change
Five findings, all in `editor/src`:

- `editor/src/src.md`: entries `assets_drag.c` (302) and `inspector.c` (305)
  are over the 300-character cap. Cut each to one sentence; anything dropped
  that the file's header does not already say moves into that header.
- `editor/src/assets_drag.h`: header comment is 65 lines, cap 60. Move the
  why of a single function from the top comment to above that function's
  declaration, or tighten, until the top comment is at most 60 lines.
- `editor/src/game_tree.h`: header comment is 61 lines, cap 60. Same remedy.
- `editor/src/inspector_layers.h` line 42: `const char **path` breaks rule
  "one level of dereference only". Change
  `voe_editor_inspector_layers_grid` to return, by value, a small struct
  holding the grid (`const voe_assets_landscape *`, NULL when none) and the
  model row's path (`const char *`), and drop the `path` parameter. Name the
  struct for what it is (e.g. the landscape a thing wears); declare it in
  `inspector_layers.h` with a one-line comment per field. Update the
  function's comment.
  - `editor/src/inspector_layers.c`: the definition (line ~27) and its own
    call (line ~69) use the new shape.
  - `editor/src/assets_drag.c`: the call at line ~249 reads the grid and
    path from the returned struct.
  These are the only callers (grep confirmed); no other folder uses this
  header.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints
no FINDING (its folder check builds `voe_editor`).
