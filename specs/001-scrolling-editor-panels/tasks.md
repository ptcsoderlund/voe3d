# 001 Scrolling editor panels — tasks

- [x] 1. `platform/` — platform reports the wheel
  - Change: A program can ask how far the wheel turned since the last poll, on both axes, in
    notches, with the same sign on both platforms. Nothing reads it yet; task 4 of this feature
    does. Decision: ADR-0153 point 10.
    1. `platform/include/platform/input.h`, beside `voe_platform_input_motion`:

       ```c
       // Wheel turn accumulated since the previous voe_platform_window_poll, in
       // notches. +x shows content further right, +y further down — the wheel turned
       // towards the person. Fractional where the device reports fractions.
       typedef struct {
       	float x;
       	float y;
       } voe_platform_wheel;

       voe_platform_wheel voe_platform_input_wheel(voe_platform_window *window);
       ```

       Replace the header paragraph *SCROLL IS NOT HERE* with: the unit (notches), the sign, that it
       drains exactly as motion does, and that how far a notch moves anything is the program's.
    2. The accumulator, `platform/src/input.h` and `platform/src/input.c`: two floats beside
       `motion_x` and `motion_y`, zeroed by `voe_platform_input_begin_poll` exactly as motion is.
       Losing focus or the pointer does not need to clear them beyond that.
    3. Wayland, `platform/src/window_wayland.c`: fill the `axis` slot of the `wl_pointer` listener
       (version 1, as every global is bound today — do not raise a bound version).
       - `WL_POINTER_AXIS_VERTICAL_SCROLL` adds `value / 10` to y;
         `WL_POINTER_AXIS_HORIZONTAL_SCROLL` adds `value / 10` to x. Wayland's positive already
         means further down and further right.
       - A named constant for the 10, with a comment: it is the length per wheel detent Weston and
         Mutter send; other compositors differ (wlroots sends 15), so a notch there is not exactly
         one; version 5's `axis_discrete` is the exact answer and is not bound.
    4. Windows, `platform/src/window_win32.c`:
       - `WM_MOUSEWHEEL`: y adds `−GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA` — Windows'
         positive is the wheel turned away, which shows content further up.
       - `WM_MOUSEHWHEEL`: x adds `GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA`.
       - Written, not verified on Windows (ADR-0130).
    5. `platform/tests/input.c`: extend the motion accumulator's case — a poll with no wheel leaves
       nought on both axes; three turns before one poll leave their sum; a poll zeroes them. Update
       the file's top comment, which names what is tested.
    6. `platform/platform.md`: the wheel in the public surface.
    7. Probe, reverted before you finish: add a temporary print of `voe_platform_input_wheel` in
       `editor/src/main.c`, run the editor on Linux, turn one notch down (towards you), then one up,
       then a sideways tilt if the mouse has one. Include the printed lines in your report, then
       revert the print. Expected: one notch towards you reads `y = 1`.

    Must not change: every existing listener, bound version and input call. `ui`, `editor`, `dev`:
    nothing reads the wheel in this task. No smoothing, no acceleration, no notches-to-lines
    conversion.
  - Covers: 7 (the wheel's sign and axes on Linux)
  - Depends on: -
  - Done when: `cmake -P check.cmake` exits 0; `cmake --build --preset debug && ctest --test-dir build/debug -R '^platform/'` passes; `git diff --quiet -- editor` exits 0

- [x] 2. `ui/` — a container may clip, and takes a scroll offset
  - Change: A container can clip its descendants to its own rectangle on X, on Y or on both, and
    can be handed an offset that moves its content. What is clipped away is neither drawn nor hit.
    Layout clamps the offset and remembers nothing. A zeroed container behaves as today. Decision:
    ADR-0153 points 5 and 6. When done, `.overflow = { VOE_UI_OVERFLOW_CLIP, VOE_UI_OVERFLOW_CLIP }`
    on a panel keeps everything inside it on screen and inside it for the pointer, and `.scroll`
    moves its content without ever running past the content's end.
    1. `ui/include/ui/layout.h`:

       ```c
       typedef enum {
       	VOE_UI_OVERFLOW_VISIBLE = 0,   // today: children stick out
       	VOE_UI_OVERFLOW_CLIP,
       } voe_ui_overflow_kind;

       typedef struct {
       	voe_ui_overflow_kind x;
       	voe_ui_overflow_kind y;
       } voe_ui_overflow;

       typedef struct {
       	...                            // existing fields unchanged
       	bool wrap;
       	voe_ui_overflow overflow;
       	voe_math_float2 scroll;        // millimetres, on CLIP axes only
       } voe_ui_container;

       // The part of a node that can be seen: its rectangle intersected with every
       // clipping ancestor's, axis by axis. Size nought on an axis when nothing is left.
       voe_ui_rect voe_ui_node_visible(const voe_ui_context *ui, voe_ui_node node);

       // The offset layout used for a container, after clamping.
       voe_math_float2 voe_ui_node_scroll(const voe_ui_context *ui, voe_ui_node node);
       ```

       Both accessors are readable in the same window as `voe_ui_node_rect` and refused in the same
       ways.
    2. Clipping, in layout (`ui/src/layout.c`):
       - A container that clips on an axis limits every descendant to its own rectangle on that
         axis — the whole rectangle, padding included. A VISIBLE axis does not narrow anything.
         Nested clips intersect.
       - Anchored descendants are clipped like any other (ADR-0153 point 5).
       - The root may clip; it is a container like any other.
    3. The offset, in layout:
       - A non-zero `scroll` on a VISIBLE axis is the caller's bug and asserts.
       - Clamp, per axis, to between nought and `measured − arranged` on that axis, nought when that
         is negative. Do it where the container's own measure and rectangle are both known and its
         children are not yet placed, so a clamped offset is never a frame late.
       - Every child of the container, in flow and anchored, is placed at minus the clamped offset on
         that axis; their descendants follow because they are placed from their parent's rectangle.
       - `voe_ui_node_measured` of the container is not changed by its offset.
    4. Widgets use the visible rectangle, `ui/src/widgets.c`:
       - Every element record's `clip` is the rectangle it uses today intersected with the node's
         visible rectangle. A record whose clip is empty on either axis is not emitted and takes no
         element capacity.
       - Hit testing tests `voe_ui_node_visible`, not the node's rectangle: a widget clipped out of
         sight is not hovered and cannot be armed or pressed.
       - A gesture already under way continues when its widget scrolls or is clipped out of sight,
         exactly as it continues past the surface's edge today (`voe_ui_pointer.over`'s paragraph).
    5. Headers:
       - `layout.h`: an *OVERFLOW* paragraph beside *OVERFLOW IS NOT SHRUNK* — VISIBLE by default,
         CLIP per axis, clip at the rectangle with padding inside it, anchored children clipped,
         offset on CLIP axes only, clamped by layout, remembered by nobody here (a scroll area
         remembers it, and arrives in task 3 of this feature).
       - `widgets.h` *WHAT IS NOT HERE*: remove *no clip narrower than a widget's own rectangle*; say
         records are clipped to the visible rectangle and clipped-out widgets are not hit.
       - `ui/ui.md`: the new public surface.
    6. Tests, `ui/tests/layout.c`:
       - A 50×20 row clipping X holding a 70 box: the box's rect is 70 wide, its visible rect 50.
       - Nested: a clip inside a clip visible-intersects on both axes; a VISIBLE-on-Y parent does not
         narrow Y.
       - An anchored child placed past a clipping parent's edge has an empty visible rect.
       - Offset: a 30-tall column clipping Y holding content 100 tall, `scroll.y = 25` → children 25
         higher, `voe_ui_node_scroll` 25; `scroll.y = 500` → 70; `scroll.y = −4` → 0; content 20
         tall with `scroll.y = 10` → 0.
       - An anchored child of a scrolled container moves with the offset.
       - `scroll.x = 1` on a container visible on X asserts, if `voe::testing` can catch an assert;
         otherwise say so in your report.
    7. Tests, `ui/tests/widgets.c`:
       - A button half clipped emits a record whose clip is the visible half; a label wholly clipped
         emits nothing and the element count says so.
       - A pointer over the clipped-away half of a button does not hover it; over the visible half it
         does.
       - A number box dragged and then scrolled out of view goes on reporting its drag.

    Must not change: a zeroed container — every existing test passes unedited. No remembered offset,
    no scrollbar, no wheel, no key table (task 3). `render`: the element record's `clip` field
    already exists and is not touched. Nothing outside `ui/` is edited.
  - Covers: 1, 3, 4 (clipping and hit testing that the editor relies on)
  - Depends on: -
  - Done when: `cmake -P check.cmake` exits 0; `cmake --build --preset debug && ctest --test-dir build/debug -R '^ui/'` passes

- [x] 3. `ui/` — a scroll area
  - Change: A widget that clips its content, remembers how far it is scrolled, draws a scrollbar
    over the content, and is scrolled by the pointer's wheel, by a thumb drag and by a press on the
    track. The wheel arrives as a length in millimetres, the form a stick or a hand will use too.
    Decision: ADR-0153 points 7–9. When done, content taller than its area scrolls under the wheel,
    the bar tracks it and can be dragged or paged, and a nested area hands on what it cannot take.
    1. `ui/include/ui/layout.h`: `voe_ui_capacities` gains `uint32_t scrolls` — how many scroll
       areas one frame may hold. Nought is allowed and means none.
    2. `ui/include/ui/widgets.h`:

       ```c
       typedef struct { bool x; bool y; } voe_ui_scroll_axes;

       // A column that clips on both axes and scrolls on the axes named. Its
       // `overflow` and `scroll` are the area's own: a caller who set either asserts.
       // Closed by voe_ui_end.
       voe_ui_node voe_ui_scroll_begin(voe_ui_context *ui, const char *name,
       				uint32_t index, voe_ui_container container,
       				voe_ui_scroll_axes axes);
       ```

       Not in this task, by CLAUDE.md rule 10: `voe_ui_scroll_by` and `voe_ui_scroll_reveal`
       (ADR-0153 point 9) have no caller until gamepad focus exists. Write step 4 so the pointer's
       length goes through one internal function that takes a starting area and a length, which
       `voe_ui_scroll_by` will later expose.

       `voe_ui_pointer` gains `voe_math_float2 scroll` — millimetres this frame, positive showing
       content further right or down; nought is no scroll, so every existing initialiser keeps its
       meaning.
    3. What is remembered: a table in the context, `scrolls` long, of widget key → offset.
       - `voe_ui_scroll_begin` claims its key like every widget and hands layout the remembered
         offset (nought for a key not in the table). After layout, the table stores
         `voe_ui_node_scroll` — the clamped offset.
       - An entry whose area was not called this frame is dropped at frame end.
       - A frame with more scroll areas than `scrolls` is refused exactly as too many nodes is: named
         on stderr, `voe_ui_frame_end` false.
       - An axis not in `axes` is clipped and its offset is always nought.
    4. Scrolling by a length, and passing it on. The innermost area takes as much of each axis as its
       clamp allows; the remainder goes to the next scroll area outward, and so on; what nobody can
       take is dropped.
       - The pointer's `scroll` starts at the innermost scroll area whose visible rectangle holds
         `pointer.at`, and only while `pointer.over`. Worked out inside `voe_ui_frame_end`, after the
         hit test, against this frame's rectangles, and lands in the next frame's layout.
    5. The scrollbar, over the content:
       - Drawn after the area's children, inside the area's rectangle: a vertical bar along the right
         edge, a horizontal one along the bottom, each only when its axis scrolls and has range
         (measured greater than arranged). When both show, each stops short of the corner by the
         other's thickness. Takes no layout space.
       - Track and thumb are solid records clipped to the area's visible rectangle. Thumb length
         along the track is `track × arranged / measured`, never less than a named minimum; its
         position is proportional to the offset.
       - Constants in `ui/src/widgets.c` beside the button's, until a theme exists: thickness 1.5 mm,
         minimum thumb 5 mm, track colour, thumb normal, hovered and held.
       - The bar is in front of the content for the pointer. Pressing the thumb holds it; moving the
         pointer by d along the track moves the offset by `d × measured / arranged`; a gesture under
         way continues off the bar and off the surface. A press on the track outside the thumb moves
         the offset one arranged length towards the pointer, once per press, and holds nothing.
    6. Header paragraphs, `widgets.h`: why the offset is remembered here and not in layout; that a
       disappearing area forgets; the pass-outward rule; that a scroll arrives as millimetres so a
       stick or a hand can produce it; that it lands next frame and why; that the bar overlays
       content, so pad the area for clearance. *WHAT IS NOT HERE*: no scrolling by a program or to a
       node yet (they arrive with focus), no dragging the content itself, no focus, no smooth
       scrolling, no saved offset. `ui/ui.md`: the new public surface.
    7. `ui/tests/widgets.c`, headless as the file already is:
       - A 30-tall area over content 100 tall: pointer `scroll.y = 10` → next frame's children 10
         higher; `scroll.y = 500` → 70.
       - Forgetting: an area skipped for one frame starts at nought when it returns.
       - Passing on: an inner area at its end hands the rest to the outer; an inner area with range
         takes it all and the outer is unmoved; an inner area scrolling Y only passes X outward whole.
       - Bar: no record when content fits; thumb length and position at offsets 0, 35 and 70; a thumb
         drag of 3 mm moves the offset by 10; a track press below the thumb moves by 30 once; a
         hovered thumb hides the button beneath it from the pointer.
       - Refusal: `scrolls = 1` and two areas → `voe_ui_frame_end` false.
       - Every test that predates this task passes unedited; a zero `scrolls` is allowed, so none
         needs it.

    Must not change: `ui/src/layout.c`'s rules as they stand after task 2 (the X-then-Y pass, wrap,
    clip and clamped offset), except reading the capacity. `platform`, `editor`, `render`. No wheel
    reading here — the caller converts to millimetres. Today's hovered and held behaviour for every
    widget not under a bar.
  - Covers: 4, 8 (scroll area, wheel length, thumb drag, track page), 5 (clamped offset remembered,
    forgotten when an area disappears)
  - Depends on: 2
  - Done when: `cmake -P check.cmake` exits 0; `cmake --build --preset debug && ctest --test-dir build/debug -R '^ui/'` passes

- [x] 4. `editor/` — the editor's panels scroll, and the inspector's rows wrap
  - Change: Nothing in the editor draws outside its column. The inspector's field rows fold onto
    further lines when the column is too narrow for a label and its number boxes. A column whose
    content is longer or wider than it scrolls with the wheel or its scrollbar. Decision: ADR-0153
    point 11. When done, the inspector's numbers are on screen at any window width, folding under
    their labels when the column is narrow and reachable by scrolling when a column is too small for
    them.
    1. Every leaf that is not a scene view is a scroll area, `editor/src/dock.c`:
       - The leaf's panel keeps its background, size and padding, and takes
         `.across = VOE_UI_ACROSS_FILL`.
       - Inside it, one `voe_ui_scroll_begin` keyed by the leaf's panel key and view, scrolling
         `{ .x = true, .y = true }`, sized grow along and filled across, holding what
         `voe_editor_panel_draw` draws today. The panel's gap moves onto the scroll area.
       - Scene-view leaves are unchanged: no scroll area around a picture.
    2. The inspector's rows fill and wrap, `editor/src/inspector.c`:
       - The scroll area's content column and each component panel fill across
         (`VOE_UI_ACROSS_FILL`), so every field row is arranged at the column's width.
       - Every row `field_row` and `rotation_rows` open takes `.wrap = true`, keeping its gap and its
         `across`. A narrow column puts the number boxes on lines beneath the label; a single box
         wider than the column is alone on its line and the area scrolls sideways to it.
       - The Scene list (`editor/src/scene.c`) gets nothing but its leaf's scroll area.
    3. The wheel, `editor/src/main.c`, where the pointer is built:

       ```c
       voe_platform_wheel wheel = voe_platform_input_wheel(window);
       ...
       	.scroll = { wheel.x * WHEEL_MILLIMETRES, wheel.y * WHEEL_MILLIMETRES },
       ```

       `WHEEL_MILLIMETRES` is a named constant of 10, with a line saying a notch's length is this
       program's to choose and `ui` is handed millimetres. The scene views keep the middle button
       and read no wheel.
    4. Capacities: wherever the editor's `voe_ui_context` is created (today `voe_ui_context_new` in
       `editor/src/interface.c`), `scrolls` covers one per non-picture leaf, with the arithmetic in a
       comment.
    5. `editor/editor.md`: the *What is deliberately absent* list loses *no scrolling, no clipping*;
       add one sentence that each non-picture column scrolls and the inspector's rows wrap.

    Must not change: the dock tree, its fractions and its seams. The scene views — their picture,
    their camera drag, and that a wheel over them does nothing. What the inspector edits, how a drag
    writes (ADR-0134, ADR-0136), selection in the Scene list. `ui`, `platform`, `render`: nothing
    but calls.
  - Covers: 1, 2, 3, 4, 5, 6, 7, 8 (hand-checked in plan.md Verification)
  - Depends on: 1, 3
  - Done when: `cmake -P check.cmake` exits 0

- [x] 5. `ui/` — a wrap revises every ancestor's measure
  - Change: When a container wraps, what it measures to changes, and every ancestor that had
    already been measured keeps the pre-wrap number. `voe_ui_node_measured` therefore lies about
    every ancestor of a wrapping container, and both readers of that number — `scroll_clamp`
    (`ui/src/layout.c:417`) and `scroll_range` (`ui/src/widgets.c:729`) — see range the content does
    not need: a scroll area draws a bar over slack that is not there and lets the wheel scroll into
    it. In the editor the phantom X range is 20–35 mm, about three quarters of the Inspector
    column, at every width at which anything wraps, the default included. Found in review of this
    feature; a probe confirmed it (a clipping column 50 wide over a wrapping row of four 20 boxes:
    row measured.x 40, outer measured.x 80). The fix is a corrective measure sweep after both
    passes. **It does not go in `widgets.c` and it does not go in `editor/`** — see step 1's last
    bullet, which is the argument and belongs in the header you write.
    1. The sweep, `ui/src/layout.c`, in `voe_ui_frame_end` between the two-axis loop and `clip()`:
       - For each axis in turn, X then Y: call `measure(ui->nodes, ui->count, axis_y)` again, then
         sweep the array forwards — parents before children, as `arrange` does — and for every
         container remember `axis(c->scrolled, axis_y)`, call `scroll_clamp(c, axis_y)`, and where
         the two differ `shift_subtree` every child, anchored and in flow alike, by
         `old - new`. That is the move `arrange_along` already makes for a wrapping column's across
         axis (`ui/src/layout.c:745-768`); this is the same move, made for every container.
       - **Measure only. No rectangle is resized and no run is broken again**, the root's rectangle
         included. The width a container wrapped at is the width it was arranged to; re-arranging X
         from the revised measure could break the lines differently, and that iteration is the one
         thing the axis order exists to rule out. So a NATURAL-width ancestor keeps its pre-wrap
         rectangle and now measures less than it — the true statement, and the one the accessor
         exists to make.
       - Parents first, because a parent's shift moves its descendants and a nested area's own
         re-clamp then adds to it. A container whose offset did not change is not touched.
       - Both axes, and each has a case behind it (CLAUDE.md rule 10). A wrapping **row** revises
         its own X in `arrange(X)`, after `measure(X)` fed its ancestors. A wrapping **column**
         revises its own Y at `layout.c:737` and its own X at `layout.c:752`, both in `arrange(Y)`,
         after `measure(Y)` fed its ancestors — and a column filled across by an `ACROSS_FILL` row
         is how such a column gets a height to break at while staying NATURAL across, which is
         exactly the shape whose parent reads the changed number. Step 5 has one test for each.
       - Idempotent everywhere else: re-measuring reads the children's `natural`, which nothing
         between the passes changed, so a tree holding no wrapping container comes out to the bit
         as it was and no offset moves. Every existing test is the proof.
       - **Why not `scroll_range` in `widgets.c`.** Satisfy yourself before you write it: the
         one-line patch there hides the bar but leaves `scroll_clamp` accepting an offset in a
         range that is not there, so the wheel still scrolls the content sideways into slack — with
         no bar to show it and nothing to scroll back by. It would also put a second copy of the
         rule beside `voe_ui_node_measured`, which would go on lying. A measure that is wrong is
         fixed where the measure is made.
    2. The file header of `ui/src/layout.c`, a paragraph in the shape of the ones around it, after
       *SINCE CARD 072 THE PAIR RUNS ONCE PER AXIS*: that a wrap is decided in `arrange` and so
       lands after the `measure` that fed the ancestors, which is what makes the sweep necessary;
       that it revises measures and never rectangles, and why; that it re-clamps and shifts, because
       an offset clamped against range that is not there is an offset the person cannot undo; and
       that a wrapping column's own revisit is this same move made early, so the sweep finds nothing
       left to do there.
    3. `ui/ui.md`: the `src/layout.c` line names the corrective sweep beside what already passes
       between the X pass and the Y pass. The public surface does not change, so nothing else does.
    4. `ui/include/ui/widgets.h:105-108`, while the file is open: *AN AREA THAT IS NOT CALLED IN A
       FRAME IS FORGOTTEN* illustrates itself with "an inspector switched to another entity and back
       starts at the top", and in this editor that is false — the area is keyed by the dock leaf, is
       called every frame, and keeps its offset across a selection change. Keep the rule, which is
       right; replace the illustration with a true one: an area that stops being called at all — a
       panel closed, a tab switched away — comes back at nought, while an area called every frame
       keeps its offset and the clamp is what brings a shorter inspector back to the top of its
       content.
    5. Tests, `ui/tests/layout.c`, among the wrapping cases and registered in `main`. The numbers
       below were worked out by hand; work them out again and report a disagreement rather than
       taking either side on trust.
       - `wrap_revises_every_ancestor`. Outer column, `fixed(30)` along and `fixed(50)` across,
         `.across = VOE_UI_ACROSS_FILL`, clipping both axes, `.scroll = { 20.0f, 0.0f }`; inside it
         a middle column, natural sizing, `.across = VOE_UI_ACROSS_FILL`; inside that a wrapping
         row, natural sizing; inside that four boxes 20 by 5. No gaps, no padding. The row is
         filled to 50 and breaks into two lines of two. Assert `measured.x` is 40 on the row, on
         the middle column **and on the outer column** — the outer's was 80, and that is the
         defect — and `measured.y` is 10 on all three. Assert `voe_ui_node_scroll(outer).x` is 0,
         the phantom 30 mm of range having gone, and the four boxes at (0, 0), (20, 0), (0, 5),
         (20, 5): before the re-clamp and shift they sat 20 mm to the left of that.
       - `wrap_column_revises_every_ancestor`, the mirror on the other axis. Outer row, `fixed(60)`
         along and `fixed(40)` across, `.across = VOE_UI_ACROSS_FILL`, clipping both axes,
         `.scroll = { 0.0f, 15.0f }`; inside it a wrapping column, natural sizing; inside that four
         boxes 10 wide by 15 tall. The column is filled to 40 tall and breaks into two lines of
         two, so it measures 30 along Y and 20 across X while its rectangle stays 10 wide. Assert
         the outer's `measured.y` is 30 — it was 60 — and its `measured.x` is 20 — it was 10, the
         wrapping column's across revision being stale in its parent the same way. Assert
         `voe_ui_node_scroll(outer).y` is 0 and the boxes at (0, 0), (0, 15), (10, 0), (10, 15).
       - The file's top comment names what each case pins down; add the pair under the wrapping
         paragraph, saying that a wrap is decided after the measure that fed the ancestors and that
         these two are what keep the accessor honest on each axis.
    6. Tests, `ui/tests/widgets.c`: extend `content_that_fits_has_no_bar` to the wrapped case —
       content that fits **because it wrapped** has no bar either. An area 40 by 30 with
       `.across = VOE_UI_ACROSS_FILL` and axes `{ .x = true, .y = true }`, holding a wrapping row of
       three boxes 15 by 8: two lines, measured 30 by 16, both inside the area, so no record at all
       and `voe_ui_element_count` is nought. Before the fix the area measured 45 wide and emitted a
       horizontal track and thumb. Update the file's top comment where it says the bar is absent
       when content fits.
    7. Regenerate both screenshots, which show the defective state — a horizontal bar over slack
       that is not there: `specs/001-scrolling-editor-panels/inspector-wraps.png` (900 by 720, rows
       folded) and `inspector-scrolls.png` (700 by 720, scrolled). A horizontal bar remains in both,
       and correctly so: the component title is a single unwrappable label about 43 mm wide against
       a column of 20 to 27 mm, and at 700 a number box is 0.93 mm too wide as well. What changes is
       the thumb, which grows as 27.3 mm of phantom range goes (measured X 72.14 to 44.80 at both
       sizes). Measured after the fact, not predicted: the original wording here said the bar would
       go entirely, and that was wrong. The throwaway that made them is `~/voe3d-scratch/capture/capture_editor.c` with
       `bgra2png.py` beside it, taking width, height, scroll in millimetres and an output path; it
       draws on `voe_render_device_new_headless`. **It stays out of the repository** (CLAUDE.md:
       throwaway spikes do not live here) — only the two PNGs are committed. Rebuild it against the
       fixed tree before capturing.

    Must not change: `scroll_range` in `ui/src/widgets.c`, which becomes true of its own accord once
    the measure is right. The X-then-Y arrangement order, `break_lines`, and every rectangle the
    passes produce for a tree with no wrapping container in it. `editor/`, `platform/`, `render/`:
    nothing at all — `git diff --ignore-cr-at-eol --quiet -- editor platform render` exits 0. Every
    existing `ui` test passes unedited; an expectation that does change is this same defect showing
    up in another case, so work the new number out by hand, correct the case's comment, and name it
    in your report — never weaken or delete a test to make it pass.
  - Covers: 3 (a horizontal bar only when something really is wider than the column), and it
    re-verifies 1 and 2 — the review defect
  - Depends on: 3
  - Done when: `cmake -P check.cmake` exits 0 on Linux (this machine builds through the scratch
    toolchain: `source ~/voe3d-scratch/env.sh`, `~/voe3d-scratch/mirror.sh`, then run it from
    `~/voe3d-scratch/tree` and never from the repository, and never edit the mirror);
    `ctest --test-dir build/debug -R '^ui/'` passes with the two new layout cases and the extended
    widgets case in it; `git diff --ignore-cr-at-eol --stat -- specs/001-scrolling-editor-panels`
    names both PNGs as changed, and in each the horizontal thumb has grown by the phantom range
    removed (see step 7 — a bar remains, and honestly)
