# 04 — Held, dragged and selected controls draw inverted
folder: ui
decisions: 0168, 0177, 0194, 0196

## Change
`ui/src/button.c`: a held button and a number box being dragged fill and border in `theme->inverse` where
they used the accent. Add `voe_ui_choice_begin`: a button (same key, press, release and answer as
`voe_ui_button_begin`, read with `voe_ui_button_action`) that also draws `inverse` while `selected`; not
selected, it is a button. Hovered and not held or selected stays `control_hovered`. Add
`bool voe_ui_control_inverted(const voe_ui_context *ui, uint32_t node)`: true when `node` is a button held
this frame, a number box being dragged, or a choice made selected; card 05's label ink asks it of a label's
nearest button or number box ancestor.

`ui/src/context.h`: the widget record gains `bool selected` (a choice's flag, set at its begin call);
declare `voe_ui_control_inverted` under "What button.c offers".

`ui/include/ui/widgets.h`: declare
`voe_ui_node voe_ui_choice_begin(voe_ui_context *ui, const char *name, uint32_t index, bool selected);`
beside the button, with a comment: one of a list, marked by inversion while `selected` (ADR-0194), closed by
`voe_ui_end`. Rewrite the button's, the number box's and the field's paragraphs that say "the accent" to say
`inverse`, and that a label inside an inverted control draws in `inverse_ink` (card 05 makes that true).
Leave `VOE_UI_TEXT_ROLE_ACCENT` alone; card 09 removes it.

Tests in `ui/tests/button.c` that expect a held or dragged fill equal to `TEST_THEME.accent` keep passing,
because card 03 made `accent` equal `inverse`; do not edit them here.

## Done when
The folder's check passes (`checks.sh` for `ui`).
