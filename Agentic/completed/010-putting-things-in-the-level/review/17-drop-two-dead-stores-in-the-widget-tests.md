# 17 — Drop two dead stores in the widget tests
folder: ui/tests
decisions: 0168

## Change
`ui/tests/widgets.c` only. check.cmake's analyser reports `Value stored to 'f' is never read` twice. Both are
the first of two builds in a row, where the first build is there only for the frame it runs:

- line 2006, in `unfocused_hands_back_the_callers_own_pointer`: `f = build_field(ui, arena, OUTSIDE_FIELD, true,
  true, hello, NO_KEYS);`
- line 2356, in `a_refused_enter_stays_open_and_escape_closes`: `f = build_numbers(ui, arena, ON_N, false,
  typing("abc"));`

In each, drop the `f = ` and keep the call as a statement, cast to `(void)` if the compiler warns about an
unused result. Change nothing else; the cases must check what they checked before. Read only the lines around
the two functions.

## Done when
`checks.sh --all` exits 0.
