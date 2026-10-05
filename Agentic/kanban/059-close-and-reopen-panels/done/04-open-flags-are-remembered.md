# 04 — Which dock panels are open is remembered
folder: editor
after: 03
decisions: 0168, 0351, 0363

## Change
0363 point 5.

- `editor/src/settings.h` and `settings.c`: `voe_editor_settings` gains `bool scene_open`, `assets_open`,
  `inspector_open`, `view_open`. Keys of the same names, each line `0` or `1`. A read overwrites a flag only
  from a line that is exactly `0` or `1`; anything else keeps the caller's. A write writes all nine keys after
  the kept lines. The header's key list, the "only replaces these five" sentence and the first-start paragraph
  say so (0363).
- `editor/src/panels.c` (and its header in `panels.h`): `voe_editor_panels_start` sets the four flags true
  before the read and sets `root->closed` from them after; `voe_editor_panels_remember` writes them from
  `root->closed` beside the sizes, so a resize keeps them.
- `editor/src/src.md`: the `settings.h` entry names the open flags.

## Done when
- The folder builds.
- This exits 0 (a capture with the Inspector and Assets closed differs from one with none closed):
  ```
  a=$(mktemp -d) && b=$(mktemp -d) && mkdir $a/voe3d &&
  printf 'inspector_open 0\nassets_open 0\n' > $a/voe3d/editor_settings &&
  XDG_CONFIG_HOME=$a build/debug/editor/voe_editor --capture $a/s.png &&
  XDG_CONFIG_HOME=$b build/debug/editor/voe_editor --capture $b/s.png &&
  ! cmp -s $a/s.png $b/s.png
  ```
- The human looks at `$a/s.png`: no Inspector and no Assets, the Scene list and views taking their room.
