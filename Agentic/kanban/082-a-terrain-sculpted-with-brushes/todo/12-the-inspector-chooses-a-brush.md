# 12 — The Inspector chooses a brush
folder: editor
after: 11
decisions: 0168, 0379

## Change
A Sculpt section in the Inspector of a thing wearing a landscape: the brush and its three numbers
(0379 point 3). Nothing sculpts yet; card 13 does.

- New `editor/src/sculpt.h` / `sculpt.c` — `voe_editor_sculpt`: `bool chosen`, `voe_3d_brush_kind kind`,
  `float radius, strength, softness` (defaults and ranges of 0379 point 3 as named constants), and room
  for card 13's hover and stroke. `voe_editor_sculpt_start` sets the defaults;
  `bool voe_editor_sculpt_wears(const voe_ecs_world *, voe_ecs_entity)` — the entity has a model row whose
  path ends `.landscape` in any case and is not a prefab's part (`voe_editor_inspector_is_part`).
  Header: the brush is the editor's, never saved or undone; chosen stays chosen across selections but
  acts only on a thing that wears a landscape.
- `editor/src/scene.h` / `scene.c` — `voe_editor_scene` gains `voe_editor_sculpt sculpt`, started where
  the scene's other state is.
- New `editor/src/inspector_sculpt.h` / `inspector_sculpt.c` —
  `voe_editor_inspector_sculpt_draw(voe_ui_context *, voe_editor_sculpt *)`: a "Sculpt" heading like a
  component's, a row of four buttons Raise, Lower, Smooth, Flatten with the chosen one drawn chosen
  (`voe_ui_choice_begin`, ui/widgets.h), and three `voe_ui_slider` rows Radius (m), Strength and Softness;
  nodes kept in the sculpt state. `voe_editor_inspector_sculpt_read(const voe_ui_context *,
  voe_editor_sculpt *)` after the frame: a fired button chooses its kind, or none when it was chosen;
  each slider's value taken.
- `editor/src/inspector.c` — after the selected thing's sections, when `voe_editor_sculpt_wears`, calls
  the draw. `editor/src/interface.c` — beside `voe_editor_inspector_buttons_read`, calls the read.
- `editor/src/frame_commands.c` — Escape, after the menus and lists it already closes, chooses no brush.
- `editor/src/src.md` — entries for the new files; inspector, interface and frame_commands entries
  name the section.

## Done when
`grep -c voe_editor_inspector_sculpt_read editor/src/interface.c` prints 1 or more, and the folder's
check passes.
Human: place any thing, set its model path in the Inspector to a `.landscape`: the Sculpt section shows;
a brush button lights and a second press clears it; Escape clears it.
