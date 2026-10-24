# 12 — The editor watches the maps its materials name
folder: editor
after: 11
decisions: 0168, 0470, 0477

## Change
0477 point 5. A map re-exported over itself has a new stamp, and nothing reads it again today. Once a
second, as the models are watched, the maps of the project's materials are asked for their stamps. A
changed map loads its materials again; card 11's queue then compresses it.

- `editor/src/map_watch.h`, `editor/src/map_watch.c` (new) — `voe_editor_map_watch`, a fixed table of
  map paths with the stamp each had at the last look, `VOE_EDITOR_MATERIALS` × 4 entries:
  - `uint32_t voe_editor_map_watch_look(voe_editor_map_watch *watch, const char *folder,
    voe_game_materials table, const char **changed, uint32_t room, voe_base_arena *scratch)`. It
    stamps every non-empty map path of the table's rows (`voe_platform_file_stamp` on the folder
    joined with the path). It writes into `changed` those whose stamp differs from the last look, at
    most `room` of them, and returns how many. A path seen for the first time is noted and never
    reported. A path no row names any more is let go.
  - `void voe_editor_map_watch_clear(voe_editor_map_watch *watch)`.
  - The header says why the editor and not `game` (the game trusts its copies), and the cost: one
    stamp a map a second.
- `editor/src/models.c` — the store keeps one watch, cleared when the folder changes. At the
  once-a-second watch, after `voe_game_models_watch`, it looks. Each changed path queues the rows
  naming it, through card 11's helper.
- `editor/src/src.md` — entries for the new files and `models.c`.

## Done when
The editor builds, and `grep -n "voe_editor_map_watch_look" editor/src/models.c` prints a line.
