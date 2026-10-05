# 01 — The panels' start and remember move out of main.c
folder: editor
after: none
decisions: 0168, 0363

## Change
`editor/src/main.c` is over 800 lines and later cards change it; this card moves one job out of it and
changes no behaviour.

- New `editor/src/panels.h` and `editor/src/panels.c`: the editor's panels as the person left them.
  - `void voe_editor_panels_start(voe_editor_dock_root *root, voe_editor_topbar *bar)` — the block in `main()`
    that sets `roots[0].tree` from `voe_editor_dock_default`, fills a `voe_editor_settings` from it, reads the
    file over it (`settings.h`) and sets the lengths, the views' share and `bar->wanted` back. `main.c` calls
    it in that block's place; its local settings variable goes if nothing else in `main()` reads it.
  - `[[nodiscard]] bool voe_editor_panels_remember(const voe_editor_dock_root *root, const voe_editor_topbar
    *bar)` — today's `voe_editor_resize_remember`, moved from `editor/src/resize.h` and `resize.c`, taking the
    root rather than the tree (card 04 adds the open flags it will write). `main.c`'s call changes to it.
- Header of `panels.h`: what it owns (start and remember), one file for the person (ADR-0220, ADR-0226), that
  card 04 adds the open flags (0363 point 5).
- `resize.h`: its remember declaration and paragraph go; the header says the sizes are remembered through
  `panels.h` when a drag ends.
- `main.c`'s header paragraph on the panels' sizes names `panels.h`.
- `editor/src/src.md`: new entries for `panels.h` and `panels.c`; the `resize.c` entry no longer says it writes
  the sizes; the `main.c` entry unchanged unless it named the settings read.

## Done when
- The folder builds.
- `test $(wc -l < editor/src/main.c) -lt 800` exits 0.
- `! grep -rq voe_editor_resize_remember editor/src` exits 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
