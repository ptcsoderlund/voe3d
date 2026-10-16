# 76 — A swap-only Make reloads without preparing
folder: editor
after: 75
decisions: 0168, 0435, 0436

## Change
0436 point 8: Make with only the swaps changed, on a model whose levels file is fresh,
saves and reloads the model at once with no "Preparing" splash; the game's loader (card 73)
patches the new swaps into the file on that reload.

- `editor/src/levels_ask.h` and `editor/src/levels_ask.c`:
  - `voe_editor_levels_ask` gains `opened`, the `voe_assets_levels` as `_open` read them.
  - New `bool voe_editor_levels_ask_swaps_only(const voe_editor_levels_ask *ask)`: `fresh`
    and either `own`, or `count`, `strength` and `shares` all as `opened` had them.
  - Header comment's example and the paragraph on Make say which answer the caller gives.
- `editor/src/session.h`: new `bool reload_due` beside `prepare_due` (near line 141): Make
  asked for the model at `prepare_path` loaded again, no levels made; main.c does it next
  frame and clears it. Its field comment says so; the header's top comment does not grow.
- `editor/src/interface_assets.c`, `voe_editor_interface_assets_levels_answer` (near line
  210): on a saved Make, `prepare_path` copied as now, then `reload_due` when
  `voe_editor_levels_ask_swaps_only`, else `prepare_due` and `prepare_remake` as now. Its
  comment names both.
- `editor/src/main.c` (near line 448): when `reload_due`, `voe_editor_models_reload(models,
  &session, gpu, session.prepare_path, scratch)` with no splash, then cleared; the comment
  above says why no splash is needed.
- `editor/src/src.md`: the `levels_ask` and `session.h` entries if they name what changed.

## Done when
`voe_editor` builds in the folder's check with no warning, and
`grep -c 'voe_editor_levels_ask_swaps_only' editor/src/interface_assets.c` and
`grep -c 'reload_due' editor/src/main.c` print at least 1.

The human's, in the editor on a project with the stable and a `.glb` with its own `_LOD1`
and `_LOD2`: every step of `## How to test` in
`Agentic/kanban/089-a-model-has-lod-levels/bugs/08-i-want-to-set-each-lods-reduction-and-swap.planned.md`,
1 to 7, and `## How to test` 1 to 6 in `Agentic/kanban/089-a-model-has-lod-levels/feature.md`
still hold.
