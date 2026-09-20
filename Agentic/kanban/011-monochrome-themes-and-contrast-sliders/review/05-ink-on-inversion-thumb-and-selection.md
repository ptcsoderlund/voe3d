# 05 — Labels on an inverted control, a held thumb and a field's selection
folder: ui
decisions: 0168, 0177, 0194, 0196

## Change
`ui/src/widgets.c`: a label's records draw in `theme->inverse_ink`, whatever its role, when its nearest
ancestor that is a button or a number box answers `voe_ui_control_inverted` (card 04, `ui/src/context.h`).
The walk is up the parent chain from the label's node, stopping at the first button or number box.
`VOE_UI_TEXT_ROLE_ACCENT` keeps drawing `theme->accent` until card 09.

`ui/src/scroll.c`: a held scrollbar thumb is `theme->inverse` where it was the accent.

`ui/src/field.c`: the whole text selected is drawn with `theme->inverse` behind it and its glyphs in
`theme->inverse_ink`, in a field and in a number box open for typing alike; the caret and unselected text
are unchanged.

## Done when
The folder's check passes (`checks.sh` for `ui`) and `grep -n "accent" ui/src/button.c ui/src/scroll.c
ui/src/field.c` prints nothing.
