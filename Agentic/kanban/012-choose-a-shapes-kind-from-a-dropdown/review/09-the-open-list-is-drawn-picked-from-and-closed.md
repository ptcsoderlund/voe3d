# 09 — The open list is drawn, picked from and closed
folder: editor
decisions: 0168, 0177, 0194, 0195, 0196, 0198

## Change
`editor/src/interface.c`, in the one function that builds a root's frame, written beside the colour picker's block
and in its shape:

- Before the frame, where `escape` already closes the picker: `escape` closes the dropdown too, then
  `voe_editor_scene_dropdown_showing(scene, &value)` says whether it shows and what value its row holds, and
  `scene->dropdown` is captured into a local exactly as `picked = scene->picking` is.
- Inside the root's frame, where the picker's anchored column is put: when it shows, an anchored child at the
  captured `left` and `top` holding one `voe_ui_surface` panel, and in it one row per value the names name —
  every index below `value_count` whose entry is not NULL, at most `VOE_EDITOR_DROPDOWN_ROWS` of them — each a
  `voe_ui_choice_begin(ui, "kind", i, i == value)` with the entry's string as a plain `voe_ui_label` inside it, so
  the value in force is marked by inversion and nothing else (ADR-0194, ADR-0196). Keep each row's node and the
  index it names in a local array of `VOE_EDITOR_DROPDOWN_ROWS`.
- After `voe_ui_frame_end`, in the same window the picker's own read is in and after
  `voe_editor_inspector_buttons_read` so that a button fired this frame opens its list rather than being closed by
  its own press: a row whose `voe_ui_button_action` fired is
  `voe_editor_inspector_named_submit(&scene->inspector, scene->world, ...)` with the captured entity, type and
  offset and that row's index, then `voe_editor_scene_dropdown_close`. A primary-button press that landed outside
  the list's rectangle closes it too, the way the picker's press-outside close is written — and a swatch or
  dropdown fired in that same frame still opens what it opened, which is the sentence the file's header already
  makes about the picker.

`editor/src/interface.h`: one more paragraph at the end of the budget, in the voice of the ones above it, and
the two constants raised. THE OPEN DROPDOWN (interface.c) ADDS THIRTY-FOUR NODES: the anchored column this file
puts round it, one; its panel, one; and up to `VOE_EDITOR_DROPDOWN_ROWS` (scene.h, 16) rows, each a choice button
and the label composed into it, thirty-two. 547 + 34 = 581. AND FOUR HUNDRED AND EIGHTEEN ELEMENTS: the panel's
border and fill, two; each row's border and fill, thirty-two; and twenty-four generous for each row's name, this
file naming none of them and neither `base` nor the declaring folder putting a length on one, 384. 5238 + 418 =
5656. Say too that it is never drawn beside the colour picker, because opening either closes the other (scene.h),
and that it is counted on top all the same. `VOE_EDITOR_INTERFACE_NODES` becomes 581 and
`VOE_EDITOR_INTERFACE_ELEMENTS` 5656; `VOE_EDITOR_INTERFACE_SCROLLS` is unchanged, the list not scrolling.

`editor/editor.md`: the Inspector's sentence about a colour being picked from its swatch gains that a value with
a named set — a shape's kind among them — is a dropdown showing the name it holds, whose list opens under it and
closes on Escape, on a press elsewhere or on a choice, and that a choice marks the project unsaved like any other
edit (0195).

## Done when
`checks.sh` for `editor` exits 0, `cmake --build --preset debug` builds the whole tree, and `voe_editor --capture
/tmp/editor.png --size 1280x720` (ADR-0177) still writes a PNG of the editor with no list on it — nothing is
selected in a captured frame, so the list is card 10's walk to see.
