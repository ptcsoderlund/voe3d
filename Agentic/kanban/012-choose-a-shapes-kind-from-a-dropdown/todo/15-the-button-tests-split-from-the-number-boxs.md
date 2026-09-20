# 15 — The button tests split from the number box's
folder: ui
decisions: 0168, 0173

## Change
`ui/tests/button.c` is 995 lines and card 17 adds cases to it. It splits in two along the seam the file already
has, by the gesture each case is about: a press, and a drag on a number box. Every case moves verbatim —
no case is rewritten, renamed, added or dropped.

`ui/tests/number.c` (new) takes the number box's drag, which is lines 315 to 750 of the file as it stands:
`PER_MM`, `START`, `struct number_frame`, `build_number`, `build_scrolled_number` and the cases
`the_number_box_is_where_the_tests_think_it_is`, `dragging_sideways_moves_the_value`,
`a_fine_drag_moves_a_tenth_as_far`, `a_drag_past_the_edge_keeps_working`,
`arriving_with_the_button_already_down_arms_no_number`, `a_number_box_that_stops_being_called_is_let_go`,
`a_number_box_scrolled_away_mid_drag_keeps_dragging` and `movement_inside_the_dead_zone_changes_nothing`. It is
standalone (rule 12): copy `SCRATCH`, the `TEST_THEME` static and `pad_all` into it rather than sharing them,
and give it a `main()` of the same shape — the shared context and theme, the cases above in the order main()
calls them today, then the second context of its own for the dead-zone case with the comment that stands over
it, and `voe_test_result()`. Its header comment is the number box's half of the one on `button.c` today: that a
drag is three calls in a row and not a mouse, that the value is handed in every frame and never kept, why
`PER_MM` is two and not one, that a box scrolled away mid-drag goes on dragging, and that typing into one is
`ui/tests/field.c`.

`ui/tests/button.c` keeps the rest: `SCRATCH`, `TEST_THEME`, `pad_all`, `struct frame`, `build`,
`build_clipped`, the press and release cases, the two clipped-button cases, and the whole STATE DRAWN INVERTED
block at its end — `build_labelled`, `check_colour`, `check_control`, `a_held_button_is_inverted`,
`a_dragged_number_box_is_inverted`, `a_selected_choice_is_inverted` and `the_inverted_states`. The dragged
number box stays here with the other two because that case is about what an inverted control draws and not
about what a drag comes to. Its `main()` loses the calls that moved and the dead zone's second context with
them. Its header comment loses the sentences about the number box's drag and says instead that the drag is
`ui/tests/number.c`, keeping every word about the four orders a hand produces and about a clip deciding what
can be hit.

`ui/tests/tests.md`: the `button.c` entry says what is left — the press and release orders, the clipped button
and the three inverted states — and a new `number.c` entry says the drag: sideways, fine, past the edge, the
dead zone and the box scrolled away mid-drag. Both name `ui/src/button.c` as the module they test, which is the
one module two programs now cover.

## Done when
`checks.sh --folder ui` exits 0; `ctest --test-dir build/debug -R "^ui/"` lists `ui/button` and `ui/number`
both passing; no case was lost —

	git show HEAD:ui/tests/button.c | grep -c VOE_TEST_CHECK
	grep -c VOE_TEST_CHECK ui/tests/button.c ui/tests/number.c

the two counts on the right add up to the one on the left; and `wc -l ui/tests/button.c ui/tests/number.c`
shows both under 800.
