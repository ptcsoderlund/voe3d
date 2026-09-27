# 24 — The Assets panel's border drags and its height is kept
folder: editor
decisions: 0168, 0279, 0226, 0232

## Change
Bug 01: the seam between the Scene list and the Assets panel ignores the pointer, because the
default tree marks that split `fixed`. Read the headers of `editor/src/dock.h`,
`editor/src/resize.h` and `editor/src/settings.h`; change these files and no others:

- `editor/src/dock.h`, `editor/src/dock.c`: drop the node's `fixed` field and its line in the
  node comment; the default tree's Scene/Assets split stays held SECOND at
  `VOE_EDITOR_DOCK_ASSETS_TALL`, no longer fixed; `voe_editor_dock_default`'s comment loses
  "on a fixed seam". Make sure `voe_editor_dock_panel_length` and `_set` with
  `VOE_EDITOR_PANEL_ASSETS` reach that column split, and with `VOE_EDITOR_PANEL_SCENE` still the
  outer split holding the left column at `SIDE_WIDE` (Scene is the column split's first child,
  so check the lookup's order). Make sure `voe_editor_dock_arrange` keeps the unheld Scene leaf
  at least `VOE_EDITOR_DOCK_PANEL_MIN` on that split, as the held Assets side already is.
- `editor/src/resize.c`: drop the `fixed` test in the border search. Confirm a double-click on
  the new border puts back the default tree's 70 mm (the way it finds the default for the other
  held seams); if it cannot, make it. `voe_editor_resize_remember` also passes the Assets length.
  `editor/src/resize.h`: its header's list of borders names the Assets seam.
- `editor/src/settings.h`, `editor/src/settings.c`: a fifth field `float assets_tall`, key
  `assets_tall`, `%.3f` mm, read when above nought and at most 1000, written with the others;
  header lists it among the kept sizes (0279).
- `editor/src/main.c`: `panel_sizes` starts `assets_tall` at `VOE_EDITOR_DOCK_ASSETS_TALL` and,
  after the read, sets it into `roots[0].tree` beside the Scene and Inspector lengths.
- `editor/src/src.md`: the `settings.h` and `resize.h` entries name the Assets panel's height /
  border; `dock.h` stays under 300 characters.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and this exits 0
(a kept `assets_tall` changes the picture):
`d=$(mktemp -d) && for h in 35 100; do mkdir -p "$d/c$h/voe3d" && printf 'assets_tall %s.000\n' $h > "$d/c$h/voe3d/editor_settings" && XDG_CONFIG_HOME="$d/c$h" build/debug/editor/voe_editor examples/tank_game --capture "$d/$h.png" || exit 1; done && ! cmp -s "$d/35.png" "$d/100.png"`
and `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.

Human's: in `examples/tank_game`, the border lights on hover and while held, drags both ways,
stops at a usable height for either panel, double-click puts it back, and the height survives a
restart and opening another project.
