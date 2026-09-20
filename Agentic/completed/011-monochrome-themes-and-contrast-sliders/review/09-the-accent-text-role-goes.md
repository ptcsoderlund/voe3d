# 09 — The accent text role goes
folder: ui
decisions: 0168, 0194, 0196

## Change
Nothing calls `VOE_UI_TEXT_ROLE_ACCENT` any more (card 08 was its last caller — confirm with
`grep -rn VOE_UI_TEXT_ROLE_ACCENT --include=*.c --include=*.h . | grep -v build`).

`ui/include/ui/widgets.h`: remove `VOE_UI_TEXT_ROLE_ACCENT` from `voe_ui_text_role`, leaving NORMAL and
SECONDARY, and reword the enum's comment: the two text lightnesses a label may ask for, the ink of an
inverted control being the control's own doing (ADR-0196).

`ui/src/widgets.c`: remove the branch that mapped that role to `theme->accent`; a label's colour is
`text_primary` or `text_secondary`, then `inverse_ink` when its nearest button or number box ancestor is
inverted (card 05).

`ui/tests/widgets.c`: delete `a_label_in_accent_draws_the_accent` and its call in `main`; the ink of a label
on an inverted control is `ui/tests/button.c`'s. Update `ui/tests/tests.md`'s `widgets.c` line.

## Done when
The folder's check passes (`checks.sh` for `ui`) and `grep -rn ACCENT --include=*.c --include=*.h ui editor
dev theme` prints nothing.
