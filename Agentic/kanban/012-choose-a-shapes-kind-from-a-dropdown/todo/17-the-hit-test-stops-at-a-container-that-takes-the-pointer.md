# 17 — The hit test stops at a container that takes the pointer
folder: ui
decisions: 0168, 0199

## Change
Card 16's field starts meaning something, in the one place the pointer is resolved — so hovering, arming, a
press, a fired release, a field's focus and a scrollbar all obey it at once, none of them deciding it a second
time.

`ui/src/button.c`:

- `hit_test` (line 231) walks paint order and keeps the last hit, which is why the loop does not stop at the
  first. Inside that loop, for the node at this position and BEFORE that node's own widget and bar tests: when
  its `blocks_pointer` is set and the pointer is inside the rectangle the widget test beside it already
  compares against — the node's visible one — the running hit goes back to nothing, `(struct voe_ui_hit){ 0 }`.
  Everything painted before it has therefore lost the pointer; everything painted after it, its own children
  among them, goes on winning it the way it does today. Nothing else in the function changes: a frame with no
  pointer present never reaches the loop, and a gesture already under way is keyed and not hit tested, so a
  drag that started before an overlay opened carries on under it exactly as it carries on past the surface's
  edge.
- Its header comment gains a paragraph directly under THE LAST WIDGET IN PAINT ORDER WINS THE POINTER, which
  that sentence now needs: AND A CONTAINER THAT TAKES THE POINTER CLEARS WHAT IS BEHIND IT. Say that a
  container declared `blocks_pointer` (ui/layout.h) is not a widget and is never itself hit — it is a hole in
  paint order: reaching it throws away whatever was hit under it, so an open overlay is solid over its whole
  outline and the cursor cannot fall through the gaps between its rows onto the fields it covers (ADR-0199).
  Say that it is tested against the visible rectangle for the same reason every widget is, so a blocker its
  scroll area clipped away blocks nothing. And say why it is here and not in four places: hover, the press that
  arms, the release that fires, the focus a press moves and the bar a press grabs are all this one hit, so
  clearing it once is all of them.

`ui/tests/button.c` (card 15 left it the press's half): one helper and four cases, in the file's own style —
geometry worked out by hand and written as numbers, no font and no device.

- `build_blocked(ui, arena, ...)`, modelled on `build_clipped`: the same two buttons in the same column, and
  after them an anchored container with `.blocks_pointer = true` and a FIXED size that covers the lower
  button's rectangle whole, holding one button of its own placed so that the lower button's centre falls in the
  blocker's padding and not on that inner button. An argument says whether the blocker is built inside a
  container that clips it away entirely, as `build_clipped` clips a button, so that the last case below is this
  helper and not a second tree.
- `a_pointer_in_a_blockers_gap_hovers_nothing`: the pointer at the covered button's centre — inside the
  blocker, on none of its children — leaves `voe_ui_button_action(ui, covered).hovered` false, and a press
  there followed by a release there leaves `.fired` false on it.
- `a_button_inside_a_blocker_is_still_hit`: the pointer over the blocker's own button hovers it, and a press
  and a release fire it. Painted after the blocker, it wins the pointer back.
- `a_pointer_beside_a_blocker_hovers_what_it_is_over`: the upper button, outside the blocker's rectangle,
  hovers and fires as it does with no blocker in the tree at all.
- `a_blocker_clipped_away_blocks_nothing`: the same tree with the blocker clipped out of sight — the covered
  button hovers and fires again, because a blocker with no visible rectangle stops nothing.

Each is called from `main()` with the shared context, beside the cases that are there.

`ui/ui.md`: the `include/ui/layout.h` entry gains, where it lists what that header says about an anchored
child, that a container may be declared to take the pointer — nothing painted under it is hovered or pressed
through it, its own children still are, and it is its visible rectangle that blocks.

## Done when
`checks.sh --folder ui` exits 0, `cmake --build --preset debug` builds the whole tree, and `ctest --test-dir
build/debug -R "^ui/"` passes with `ui/button` running the four new cases — `grep -c "^static void
a_pointer_in_a_blockers_gap_hovers_nothing\|^static void a_button_inside_a_blocker_is_still_hit\|^static void
a_pointer_beside_a_blocker_hovers_what_it_is_over\|^static void a_blocker_clipped_away_blocks_nothing"
ui/tests/button.c` prints 4.
