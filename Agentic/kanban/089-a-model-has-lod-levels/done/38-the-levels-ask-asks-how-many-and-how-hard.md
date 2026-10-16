# 38 — The levels question asks how many and how hard
folder: editor
after: 33, 37
decisions: 0168, 0430

## Change
0430 point 6: a panel "Levels of <file>" with the model's count and strength to choose, then Make or
Cancel. New files, shaped as `editor/src/assets_ask.h` / `assets_ask.c` (the Delete question: open,
draw, read after the frame, an anchored panel over the dock); the chosen button lit as
`editor/src/inspector_sculpt.c` lights its chosen kind. Card 39 wires it in; nothing calls it yet.

- `editor/src/levels_ask.h` (new): header comment: what the panel shows and asks, 0430 points 2, 3
  and 6; the values read from the model's `.parts` (`assets/include/assets/parts.h`) on open, the
  defaults when none; Make's save writes them back; nothing here prepares or closes on its own; a
  usage example as assets_ask.h's. Declares:
  - `voe_editor_levels_ask`, zeroed never asked: `bool open`; `char path[VOE_3D_MODEL_PATH]` (the
    project-relative `Assets/….glb`, as `session.prepare_path` holds it); `voe_assets_levels
    levels`; the title's kept text; nodes for the panel, four count buttons, three strength
    buttons, Make and Cancel.
  - answer enum `VOE_EDITOR_LEVELS_ASK_NONE`, `_MAKE`, `_CANCEL`.
  - `void voe_editor_levels_ask_open(voe_editor_levels_ask *ask, const char *project_folder,
    const char *path, voe_base_arena *scratch)`: the `.parts` beside `path` (its `.glb` ending, any
    case, made `.parts`) read for its levels, quietly, else the defaults; scratch rewound.
  - `_close`.
  - `void voe_editor_levels_ask_draw(voe_ui_context *ui, voe_editor_levels_ask *ask, float top)`:
    "Levels of <file name>"; a row "Levels" with "1" to "4"; a row "Strength" with "Gentle",
    "Medium", "Strong"; Make and Cancel; the chosen count and strength lit.
  - `voe_editor_levels_ask_answer voe_editor_levels_ask_read(const voe_ui_context *ui,
    voe_editor_levels_ask *ask, bool pointer_down, voe_math_float2 at)`: a fired count or
    strength button sets `ask->levels` and answers NONE; Make, Cancel, or a press outside the
    panel (CANCEL) as assets_ask's read does.
  - `[[nodiscard]] bool voe_editor_levels_ask_save(const voe_editor_levels_ask *ask, const char
    *project_folder, voe_base_arena *scratch)`: the `.parts` read (none: an empty one, count 0),
    its `levels` set, written back with `voe_assets_parts_write` as `editor/src/model_import.c`'s
    `text_written` writes one; false with a line on stderr when the write fails; scratch rewound.
- `editor/src/levels_ask.c` (new): the above, with a header comment.
- `editor/src/src.md`: entries for both files, one sentence each.

## Done when
`voe_editor` builds in the folder's check with no warning (the new file compiled, unused).
