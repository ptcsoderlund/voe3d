# 01 — Split ui/src/widgets.c by widget
folder: ui
decisions: 0168, 0173

## Change
`ui/src/widgets.c` is 2402 lines; later cards of this feature change the button, the field and the scroll
area in it. Move code only, changing no behaviour, into four files, each opening with a header comment that
says what it holds and why (the style of `ui/src/colour.c`'s):

- `ui/src/widgets.c` keeps the hashed keys and `voe_ui_widget_claim`, the theme mechanism, the pointer and
  keyboard setters, the frame boundaries with the hit test and the drags, the walk that emits records in
  paint order and dispatches per widget, the element readback, the panel, the label (its creation and its
  records), and the image.
- `ui/src/button.c` (new): the button and the number box — their begin calls, `voe_ui_button_action`,
  `voe_ui_number_action`, their dragging answer and their records (fill, border, colour by state).
- `ui/src/field.c` (new): the field, the one keyboard focus, Tab order, commit and cancel, typing into an
  open number box, and their records (caret, selection, text).
- `ui/src/scroll.c` (new): the scroll table, the offsets, the bars and their records.

A function one file calls in another is declared in `ui/src/context.h` under a comment "What <file> offers
…", as colour.c's and widgets.c's entry points are today; the rest stay `static`. If a part is still over
~800 lines, move more of what it holds by function into the part it belongs with, not a fifth file.
Update `ui/src/src.md`'s `widgets.c` entry and add the three new ones. Read `ui/src/context.h`'s header
first; read no other file but these.

## Done when
The folder's check passes (`checks.sh` for `ui`), `wc -l ui/src/widgets.c ui/src/button.c ui/src/field.c
ui/src/scroll.c` shows each under 900, and `git diff --stat` touches no file outside `ui/src/`.
