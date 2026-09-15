# 001 Scrolling editor panels — acceptance

## Start it

1. Get the tools on your path and build, from the repository root:

       cmake --preset debug
       cmake --build --preset debug --target voe_editor

2. Run it:

       ./build/debug/editor/voe_editor

3. Click an entity in the **Scene** list on the left — `Cube` will do — so the **Inspector**
   on the right fills with its transform. Most of what follows needs something selected.

## Try this

1. **Every number is in its column.** At the window's normal size, look at the Inspector.
   Expected: every number box is inside the column. Nothing spills over the edge into the
   3D view or off the side of the window.

2. **Narrow the window and the rows fold.** Drag the window's right edge leftwards.
   Expected: `position`'s three number boxes stop sitting side by side and fold onto lines
   underneath the word `position`. Widen it again and they go back onto one line.

3. **Narrow it further and you can scroll sideways.** Keep narrowing until a single number box
   is wider than the column. Expected: a thin bar appears along the bottom of the column, and
   scrolling sideways — tilt the wheel, or drag that bar — reaches the box.

   Note: a short bar along the bottom is there at normal size too. That is not a fault. The
   component's title (`voe_scene_transform`) is one long unbreakable word, about twice the
   column's width, so it genuinely does not fit and genuinely needs scrolling to read. Widen
   the window enough and the bar disappears. Whether such a title should instead be cut short
   with an ellipsis is a separate question — tell me if it bothers you.

4. **Scrolling up and down.** With the Inspector's content longer than the column:
   - Turn the wheel with the pointer over the Inspector. Expected: the content moves.
   - Drag the bar's thumb down the right edge. Expected: the content follows the thumb.
   - Click the track above or below the thumb. Expected: it jumps by one column's height,
     once per click.
   - Drag a number box left and right. Expected: it still edits the value as it always did,
     and keeps editing even if the drag carries off the column.

5. **A smaller entity starts at the top.** Scroll the Inspector to the bottom, then pick an
   entity with fewer components — `Marker`. Expected: the column shows its content from the
   top, with no blank space above it.

6. **The 3D views are untouched.** Turn the wheel with the pointer over a 3D view.
   Expected: nothing happens. The picture and the middle-button camera drag work exactly as
   they did before.

7. **The wheel goes the right way.** One notch of the wheel towards you over the Inspector.
   Expected: the content moves up, bringing what was below into view — the same direction as
   every other program. A sideways tilt scrolls sideways.

8. **The Scene list scrolls too.** Make the window short, or the Scene list's column small
   enough that its entities do not all fit. Expected: it scrolls the same way the Inspector
   does.

If you would rather see two of these before building anything, `inspector-wraps.png` and
`inspector-scrolls.png` in this folder are the folded state and the scrolled state as the
current code draws them.

## What is not delivered

Nothing was left undone: all five tasks are finished and every acceptance criterion in the
spec is met. What the spec deliberately excluded is still excluded — no scrolling by keyboard
or gamepad, no dragging the content itself, no smooth scrolling, no remembered scroll position
between runs, no colour theme for the bar, and no arrays in the inspector.

Checked on Linux only, as the spec says. The wheel is written for Windows but nobody has run
it there; if it is wrong on Windows it arrives later as a report.

## Results
