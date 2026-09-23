# 05 — The views' border is drawn in two stripes
folder: editor
decisions: 0168, 0194, 0196, 0229, 0230

## Change
Bug 01: the seam between the two scene views shows the ground, which in a dark theme is the views' own
near-black, so it cannot be seen. Fill it with a dark and a light stripe (0230). Nothing else moves: the
seam keeps its size and place, so `voe_editor_dock_arrange`, the views' room and resize.c's hit band are
untouched.

- `editor/src/dock.h`
  - `voe_editor_dock_walk` gains `const voe_ui_theme *palette` (include `<ui/theme.h>`): the palette in
    force, read only for the views' seam. Its comment says so, beside the `scene`/`views` paragraph.
  - The header's "held length" paragraph (or a short one after it) says the views' seam is drawn as two
    stripes so it shows over any picture (0230).
- `editor/src/dock.c`
  - `voe_editor_dock_walk` passes `palette` down to `walk_node`, which takes it too.
  - In `walk_node`, for the split whose two children are both SCENE_VIEW leaves (the one `views_split`
    finds): where the seam is left empty today, emit a box of the seam's size (`SEAM` along the split,
    filling across) holding two `voe_ui_swatch` calls (`ui/colour.h`), each `SEAM / 2` along and filling
    across — the first child's side `palette->inverse_ink`, the second's `palette->inverse`, each as its
    `.xyz`. The two children still get exactly the sizes the arrangement gives them, so the box plus the
    children add up to the node's length as before. Every other split's seam stays an empty gap.
  - The file header's "THE SEAM IS A GAP AND NOT A DRAWN DIVIDER" paragraph: true for every seam but the
    views', which is drawn as two stripes because both views clear to near-black whatever the theme and a
    picture may match any one colour; the pair is `inverse`/`inverse_ink`, legible by construction and on
    the theme's one hue (0194, 0196, 0230).
- `editor/src/interface.c` — the one call of `voe_editor_dock_walk` passes
  `&voe_editor_themes_chosen(themes)->palette` (`themes` is already a parameter of
  `voe_editor_interface_draw`).
- `editor/src/src.md` — the `dock.h` entry names the views' seam drawn as two stripes.

Read `dock.h`, `dock.c`'s file header and the heads of `walk_node`, `views_split` and
`voe_editor_dock_walk`, `ui/include/ui/colour.h`'s swatch declaration, and the call site in `interface.c`;
no other file.

## Done when
The coder: `bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0, and with `XDG_CONFIG_HOME` at an
empty scratch folder, `voe_editor <scratch>/p --capture <scratch>/a.png --size 1280x720` writes its picture.

The human, at a running `voe_editor`: the border between the two views shows as a dark and a light line in
Near black, in Near white and in a monochrome theme, with a view showing light and then dark content on
either side; it stays visible while dragged; the pointer still turns to the up-down arrow as close to the
border as before.
