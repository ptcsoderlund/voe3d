# 40 — No tree file in Assets, disk, check or Docs
folder: editor
after: 39
decisions: 0168, 0520

## Change
Take out of `editor/` what these commits added, seen with `git show <hash> -- editor`: card 09 `80a203ac`
(Assets: Create → Tree), card 15 `676d8897` (species files walked, followed and taken from disk), card 16
`5b85bcea` (check and Docs know the species file), and card 14 `b671167f`'s drop of a `.tree` file onto a
foliage kind. Later cards' changes to the same files stay.

Files, all in `editor/src/`: `assets_manage.c`, `assets_manage.h`, `assets_menu.c`, `assets_menu.h`,
`interface_assets.c`, `assets_walk.c`, `assets_walk.h`, `disk_take.c`, `disk_take.h`, `disk_watch.c`,
`disk_watch.h`, `file_steps.c`, `file_steps.h`, `check.c`, `check.h`, `docs.c`, `docs.h`, `assets_drag.c`,
`assets_drag.h`. A `.tree` file is then to each of them what any file of an unknown extension is. Docs'
file-kind table loses its `*.tree` row. Header points and `editor/src/src.md` entries naming species go.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -nE 'species|\.tree"'` over the files above prints nothing.
