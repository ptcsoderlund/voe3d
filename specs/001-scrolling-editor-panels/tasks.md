# 001 Scrolling editor panels — tasks

- [ ] 1. `platform/` — platform reports the wheel
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

- [ ] 2. `ui/` — a container may clip, and takes a scroll offset
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

- [ ] 3. `ui/` — a scroll area
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

- [ ] 4. `editor/` — the editor's panels scroll, and the inspector's rows wrap
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
