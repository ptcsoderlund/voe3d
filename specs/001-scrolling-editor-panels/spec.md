# 001 Scrolling editor panels

Status: accepted
Approved: 2026-09-14
Accepted: 2026-09-15

Nothing in the editor draws outside its column any more. When a column is too narrow, the
inspector folds a field's number boxes onto lines under its label. When a column is too small for
its content, it scrolls with the mouse wheel or a scrollbar. The sponsor wants every value in the
inspector reachable at any window size.

## Acceptance criteria

1. At the default window size, with an entity that has a transform selected, every number box is
   inside the Inspector column.
2. Narrowing the window folds a vector field's number boxes onto lines under its label; widening it
   puts them back on one line.
3. Narrowing further, until a single box is wider than the column, shows a horizontal scrollbar,
   and scrolling sideways reaches the box.
4. When the window is short enough that the inspector overflows, the wheel scrolls it, dragging the
   thumb scrolls it, and a press on the track pages it by one column height. Dragging a number box
   still edits its value.
5. After scrolling to the bottom, selecting an entity with fewer components shows the column from
   the top of its content, with no empty space above.
6. The wheel over a scene view does nothing, and the scene views' picture and camera drag work as
   before.
7. One notch of the wheel towards you scrolls content up into view, and a sideways tilt scrolls
   sideways, on Linux.
8. The Scene list column scrolls the same way when it has more entities than fit.
9. `cmake -P check.cmake` exits zero on Linux.

## Out of scope

- Moving focus or scrolling with a gamepad or keyboard, or scrolling to a chosen item.
- Dragging the content itself, smooth scrolling, and remembering a scroll position across runs.
- Showing arrays in the inspector.
- A theme for the scrollbar's colours.
- Verifying on Windows: the wheel is written for Windows but checked only on Linux.

## Constraints

- Linux first: acceptance is on Linux; Windows problems arrive later as reports.

## Defaults

- One wheel notch scrolls 10 mm on GNOME and Weston. Other Wayland desktops report a notch
  differently, so on wlroots desktops such as Sway it is about 15 mm; the exact per-notch reading
  is left for later.
- The scrollbar is 1.5 mm thick and drawn over the content, not beside it, with a thumb at least
  5 mm long.
- A nested scroll area passes on whatever scroll it cannot use to the area around it.
- A column that disappears forgets its scroll position.

## Open questions

- None.
