# 51 — The editor builds on groups
folder: editor
after: 50, 47
decisions: 0168, 0520, 0521, 0516, 0517, 0442, 0476, 0481, 0511

## Change
The editor moves to groups with what a kind did today; card 52 shows each glb. Each group acts through its
first glb where a kind acted through its path.

- `editor/src/models.h`, `models_edit.c`: `voe_editor_models_kind_set`, `_look_set`, `_grow_set` take a
  group and glb where they took a kind, calling 3d's `voe_3d_models_landscape_glb_set`, `_look_set`,
  `_grow_set` (`3d/include/3d/models_foliage.h`).
- `editor/src/strokes.h`, `strokes.c`: a look stroke names its group and glb; `look_before` and
  `look_after` are that glb's look.
- `editor/src/sculpt.h`, `sculpt.c`: arrays per kind are per `VOE_ASSETS_LANDSCAPE_GROUPS`; picked bits
  are per group; the look ask names group and glb (glb 0 for now).
- `editor/src/inspector_foliage.h`, `inspector_foliage.c`: a folding box per group titled as 0521 point 6
  says; inside, glb 0's settings as a kind's are today.
- `editor/src/inspector_layers.h`, `inspector_layers.c`: a grow row per group, titled as its box.
- `editor/src/assets_drag.h`, `assets_drag.c`: the add row makes a group; a drop on a group's title
  replaces its glb 0 for now.
- `editor/src/folds.h`, `folds.c`: a group's fold is `group <G> <path>`; `_kind_removed` and
  `_kind_returned` become `_group_removed` and `_group_returned`. Old `kind` folds are left unread.
- `editor/src/game_tree_landscapes.c`: cooks `groups` and `group_count`, each look by field name, grows by
  group.
- `editor/src/docs.c`: its Landscape files text says group where it says kind.
- Any other file the grep below finds (`landscape_panel.c`, `inspector_sculpt.c`).

Header points naming kinds say groups. Entries in `editor/src/src.md`.

## Meaning
- Scout: a drop on a kind row replaces the path keeping its look; the add row adds a kind with a new seed,
  refused at 16, with no undo step. Nothing stops a path twice.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -rnE 'LANDSCAPE_KINDS|KIND_PATH|kind_count|kind_set|kind_seed|kind_remove|folds_kind' editor/src`
prints nothing.
