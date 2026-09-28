# 07 — The Scene list has a file of its own
folder: editor
decisions: 0168

## Change
A move, no behaviour change: `editor/src/dock.c` is 766 lines and cards 08–09 grow the Scene
list, so the list leaves it first.

- New `editor/src/scene_list.h`, `editor/src/scene_list.c`: `void voe_editor_scene_list_draw(
  voe_ui_context *ui, voe_editor_scene *scene);` — `dock.c`'s `static scene_panel`, moved whole,
  with its three comment paragraphs (the list is the identity table, rows keyed by name and
  index, Add entity above the list) as the new header's points.
- `editor/src/dock.c`: `voe_editor_panel_draw` calls `voe_editor_scene_list_draw`; the static
  function and its comment go; includes that only it needed go with it.
- `editor/src/src.md`: `scene_list.h` and `scene_list.c` entries after `dock.c`; `dock.c`'s entry
  no longer implies it draws the Scene rows.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, `grep -c
scene_panel editor/src/dock.c` prints 0, and `wc -l < editor/src/dock.c` is under 740.
