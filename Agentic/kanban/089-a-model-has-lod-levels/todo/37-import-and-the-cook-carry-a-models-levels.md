# 37 — Import and the cook carry a model's levels
folder: editor
after: 33
decisions: 0168, 0420, 0430

## Change
0430 point 4: `voe_assets_parts` now holds `levels` (`assets/include/assets/parts.h`, card 33), and its
write asserts a count of 1 to 4, so every `.parts` the editor writes must carry them.

- `editor/src/model_import.c`: in `voe_editor_model_import` (line 252), `parts.levels` is set before
  the write to the levels of the `.parts` already at `parts_path_of(glb, …)` when one reads and
  parses, else `VOE_ASSETS_LEVELS_DEFAULT`: a re-import keeps a person's choice. One small static
  helper above it does that read quietly (no notice, no stderr line of its own); `parts_read`
  (line 292) is left as it is. Header: the sentence on `<stem>.parts` says it keeps the levels of
  the one there, else the defaults.
- `editor/src/model_import.h`: the same in the comment on the import, one phrase.
- `editor/src/game_tree_materials.c`: the parts entry's text (the format near line 152) gains
  `.levels = { .count = %u, .strength = VOE_ASSETS_LEVELS_<GENTLE|MEDIUM|STRONG> }` after the
  materials, the enum's name written by a small static helper over the three. The header's example
  shows it.
- `editor/src/src.md`: the `model_import` and `game_tree_materials.c` entries name the levels, one
  phrase each, each entry still one sentence.

## Done when
`voe_editor` builds in the folder's check with no warning, and
`grep -n 'VOE_ASSETS_LEVELS' editor/src/model_import.c editor/src/game_tree_materials.c` prints lines
in both.
