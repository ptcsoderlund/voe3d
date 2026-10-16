# 30 — The Assets menu makes a model's levels
folder: editor
after: 29
decisions: 0168, 0426, 0429

## Change
0429 point 2: right-clicking a `.glb` in Assets offers Make levels, after Import textures. It runs
the splash wait Import's prepare already runs (`session.prepare_due`, card 11), with the remake
card 29 added to `voe_game_models_prepare` (`game/include/game/models.h`). The model in the store
is not loaded again (0429 point 4).

- `editor/src/loading.h` / `loading.c`: `voe_editor_loading_prepare` gains `bool remake`, passed to
  `voe_game_models_prepare`; its comment says Import leaves it false and Make levels sets it. The
  header's PREPARE sentence names Make levels.
- `editor/src/session.h`: beside `prepare_due`, `bool prepare_remake`; the comment there says the
  prepare is asked by an import of a `.glb` or by Make levels, which also sets `prepare_remake`,
  both cleared by main.c once the wait has run.
- `editor/src/main.c`, the `session.prepare_due` block (around its line 447): `.remake =
  session.prepare_remake` in the context, and `prepare_remake` cleared with `prepare_due`. 762
  lines: add only these two lines.
- `editor/src/assets_menu.h` / `assets_menu.c`: a new last item `VOE_EDITOR_ASSETS_MENU_MAKE_LEVELS`,
  drawn "Make levels" after Import textures on a `.glb`'s row only; `VOE_EDITOR_ASSETS_MENU_ROWS`
  5; the row-table in the `.c` (beside line 25) and the assert on the last item (line 253) follow.
  The header's first paragraph, the ROWS comment and `_open`'s comment name the row with 0429.
- `editor/src/interface_assets.c`: `voe_editor_interface_assets_menu_do` carries out the new item
  as Import textures is guarded (a project folder, a selected path): the path joined under
  "Assets" as `voe_editor_interface_assets_textures_do` joins it, copied into
  `session->prepare_path` when it fits `VOE_3D_MODEL_PATH`, then `prepare_due` and
  `prepare_remake` set; a path that does not fit does nothing. The header's menu paragraph
  names Make levels and that main.c runs it behind the splash next frame.
- `editor/src/src.md`: the `assets_menu.h`, `loading.h` and `interface_assets.c` entries name
  Make levels, each in a phrase, keeping each entry one sentence.

## Done when
`voe_editor` builds in the folder's check with no warning, and
`grep -n 'MAKE_LEVELS' editor/src/interface_assets.c editor/src/assets_menu.c` prints lines in
both. The human: open a project whose `.glb` was imported before 089, right-click it in Assets →
Make levels; the splash shows "Preparing <file>" once and
`<project>/Cache/levels/Assets/<file>.levels` has a new time. Place it, fly away: the Inspector's
level goes up (How to test step 2 on a model imported earlier).
