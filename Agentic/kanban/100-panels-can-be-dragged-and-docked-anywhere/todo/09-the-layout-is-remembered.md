# 09 — The layout is remembered in editor settings
folder: editor
after: 08
decisions: 0168, 0439, 0491, 0220, 0226, 0363

## Change
0491 point 7: the tree and the floats are kept as lines and come back at start; a bad tree means
today's layout.

- `editor/src/dock.h` and `dock.c`:
  `bool voe_editor_dock_check(const voe_editor_dock_tree *tree, const voe_editor_dock_float floats[VOE_EDITOR_CLOSABLE_COUNT])`
  — 0491 point 7's check, the walk an explicit stack (rule 14: the tree came from a file).
- `editor/src/settings_layout.h` and `.c` (new; `scene.h` includes `settings.h`, so `settings.h`
  cannot include `dock.h`):
  - `bool voe_editor_settings_layout_read(voe_editor_dock_tree *tree, voe_editor_dock_float floats[VOE_EDITOR_CLOSABLE_COUNT])`
    — each `dock_float` line that parses over that panel's entry; the `dock_root` and `dock_node`
    lines into `tree`, true only when a root line was read and every node line parsed. It does not
    check the tree.
  - `[[nodiscard]] bool voe_editor_settings_layout_write(const voe_editor_dock_tree *tree, const voe_editor_dock_float floats[VOE_EDITOR_CLOSABLE_COUNT])`
    — every other key's line kept in its order, then the root, the nodes and one float line per
    moving panel.
  - Header: the line forms and words of 0491 point 7, a first start not a failure (as `settings.h`).
- `editor/src/settings.h` and `.c`: the file's line read and keep-other-keys write that the panel
  and snap writes share become callable by `settings_layout.c`, under a "not for other callers"
  section, rather than copied.
- `editor/src/panels.h` and `.c`: start reads the layout over the default floats; a tree that reads
  and passes `voe_editor_dock_check` is the root's tree and 0226's size keys are not applied; else the
  default tree with them as today. Remember writes the layout after the sizes. The header says so.
- `editor/src/src.md`: `settings_layout.h`, `settings_layout.c`; the `panels.h` and `dock.h` entries.

## Done when
This exits 0 (a moved layout draws differently; a broken one draws today's):
```
d=$(mktemp -d) && e=build/debug/editor/voe_editor && mkdir -p $d/m/voe3d $d/b/voe3d &&
printf 'dock_root 0\ndock_node 0 split row second 48.000 0.500000 1 2\ndock_node 1 split column fraction 0.000 0.500000 3 4\ndock_node 2 leaf scene\ndock_node 3 leaf view 0\ndock_node 4 leaf view 1\ndock_float assets 1 0.000 0.000 60.000 40.000\ndock_float inspector 1 70.000 0.000 60.000 60.000\n' > $d/m/voe3d/editor_settings &&
printf 'dock_root 9\n' > $d/b/voe3d/editor_settings &&
XDG_CONFIG_HOME=$d $e --capture $d/a.png && XDG_CONFIG_HOME=$d/m $e --capture $d/m.png &&
XDG_CONFIG_HOME=$d/b $e --capture $d/b.png && cmp -s $d/a.png $d/b.png && ! cmp -s $d/a.png $d/m.png
```
