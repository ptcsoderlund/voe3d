# 13 — The question: load from disk, or keep mine
folder: editor
after: 12
decisions: 0168, 0193, 0506, 0509

## Change
- New files `editor/src/disk_ask.h` and `editor/src/disk_ask.c`, following the pattern of
  `assets_ask.h`. Read its header and `textures_ask.h`'s first.
  - `voe_editor_disk_ask` holds `open`, the file's line and two buttons.
  - Calls: `_open(ask, path)`, `_close`, `_draw(ui, ask, top)`, and
    `_read(ui, ask, pointer_down, at)`, which returns none, load or keep.
  - The panel names the project-relative file and offers two choices: load from disk, and keep
    mine.
  - Escape does not answer it.
- `editor/src/session.h`: the session holds `voe_editor_disk_ask disk_asking` beside
  `textures_asking`, plus a field for its last answer, which card 14 reads.
- `editor/src/interface.c`, `interface.h`, `interface_assets.c` and `interface_assets.h`: draw it,
  read it and let it block other input exactly as `textures_asking` does; grep for that name in
  each file.
  - When answered, the answer is stored in the session and the question closed.
  - interface.h's element budget counts the panel's nodes, as it does the textures question's.
- `editor/src/src.md`: an entry for the new files.

## Done when
- `grep -q disk_asking editor/src/interface.c` and `grep -q voe_editor_disk_ask_read editor/src/disk_ask.h`
  both exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
