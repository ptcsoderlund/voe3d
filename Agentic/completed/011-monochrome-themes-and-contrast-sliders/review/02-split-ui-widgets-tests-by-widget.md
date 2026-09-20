# 02 — Split ui/tests/widgets.c by widget
folder: ui
decisions: 0168, 0173

## Change
`ui/tests/widgets.c` is 2641 lines. Move its cases, unchanged, into four test programs matching card 01's
split, each with its own `main()` calling its cases and a header comment saying what it pins:

- `ui/tests/widgets.c` keeps keys, duplicate keys, panels, labels, clipping, the known tree emitted as a
  known list, the nearest theme winning and the unbalanced push.
- `ui/tests/button.c` (new): press and release in every order, the sideways drag, the clipped button, the
  number box's drag and dead zone.
- `ui/tests/field.c` (new): the field's focus, typing, Backspace, Enter, Escape, Tab, capacity, selection,
  `voe_ui_typing`, and typing into a number box.
- `ui/tests/scroll.c` (new): the scroll area's remembering and bars.

Each program is standalone (rule 12): copy the helpers and the `TEST_THEME` set-up a program needs into it,
and no more. Update `ui/tests/tests.md`'s `widgets.c` entry and add the three new ones.

## Done when
The folder's check passes (`checks.sh` for `ui`), `ctest --test-dir build/debug -R "^ui/"` lists the
`button`, `field` and `scroll` tests passing, and the total count of `VOE_TEST_CHECK` lines across the four
files equals the count in the old `ui/tests/widgets.c` (`git show HEAD:ui/tests/widgets.c | grep -c
VOE_TEST_CHECK`).
