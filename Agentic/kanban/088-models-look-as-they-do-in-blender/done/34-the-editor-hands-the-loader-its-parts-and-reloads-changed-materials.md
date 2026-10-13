# 34 — The editor hands the loader its parts and reloads changed materials
folder: editor
after: 33
decisions: 0168, 0399, 0404, 0420

## Change
Mends the editor after cards 31 and 33, and makes a rewritten `.material` show at once (0403's
re-import and Import textures, cards 36 and 40, rewrite them). Read `editor/src/materials.h` and
`editor/src/materials.c` as the pattern for the new table.

- `editor/src/parts.h`, `editor/src/parts.c` (new, header comments): the project's `.parts` files as
  game/materials.h's `voe_game_parts` table: `voe_editor_parts` with rows, path bytes and count,
  `voe_editor_parts_read(parts, folder, scratch)` reading every `.parts` under `Assets/` with
  `assets/parts.h` (each row's path the `.parts` path with `.glb` in place of `.parts`), a bad file left
  out with one stderr line, and `voe_editor_parts_table`. Cap `VOE_EDITOR_PARTS` (128).
- `editor/src/models.h`, `editor/src/models.c`: the store holds a parts table beside the materials;
  `voe_editor_models_materials_read` reads both. After that read, every material whose row is new or
  whose values differ from before and that the store holds is loaded again, maps and all, at the next
  update (more than one may wait). The update runs models update, watch, then
  `voe_game_models_materials` with both tables, so a reloaded model wears its parts before it is
  drawn. The header's MATERIALS paragraph says so.
- `editor/src/assets_drag.h`, `editor/src/assets_drag.c`: a `.material` over a thing gives it to the
  thing's shape only; the model's Materials field case goes (0404), and over a model it drops nothing
  ("Can't drop here").
- `editor/src/src.md`: entries for `parts.h`, `parts.c`, and the changed ones (each at most 300
  characters).

## Done when
`voe_editor` builds in the folder's check with no warning.
