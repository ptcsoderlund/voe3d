# 034 — the first widgets: panel, label, button

status: review
claimed-by: claude-opus-5
blocked-by: -

**AMENDED 2026-09-10, in flight, on the coder's own finding — ADR-0106.** The
verify list below said *"`ui` tests, plain C, no graphics card"*. That is true of
every check on it **but one**: the text-scale check measures a real string, a
measurement goes through `voe_text_font`, and building a font uploads an atlas and
so needs a device. **The coder was right to stop and ask rather than guess.**

**The resolution: that one check creates a headless device** —
`voe_render_device_new_headless` — and every other check on the list stays exactly
as written. `text` is *not* being split; that was the tech lead's recommendation and
the principal overruled it, because the case for it rested on machines with no
graphics driver and every machine in use answers Vulkan. **The property the card
actually cared about is untouched: no window, no compositor, no window system.**

**And the skip message is part of this card now.** If the driver is missing the
check skips and the skip is a pass — but under ADR-0106 it must name *what went
unchecked*, not only why: `skip: no graphics driver — the text-size check did not
run`, not `skip: no graphics driver`. One line, and it is the whole content of the
rule.

Written by the tech lead under the standing grant. **Not a spin-off**: it is the
number the GUI plan reserved for this step. **Its floor is now complete** — 030 the
element buffer, 031 glyphs as elements, 033 the `ui` folder and layout, all accepted
— so it is written against what exists rather than against what was planned, which
is the rule this plan runs on.

**It does not depend on 032 or 041**, both of which are on the board. 032 puts a
panel in the world; this card draws into the surface the exhibit already has. 041
adds anchored children; this card emits in whatever order layout gives it, so it
gains anchoring for free the day 041 lands and needs nothing from it before then.

The decisions behind this card: **ADR-0091** (immediate mode), **ADR-0092** (one
draw from an element buffer, paint order is submission order), **ADR-0093** (`ui` is
a leaf and is *given* a pointer as a two-dimensional point), **ADR-0090** (a size is
measured from the font in hand, never hard-coded), **ADR-0099** (the GUI surface runs
Y down from the top-left — the same space the element record uses), **ADR-0102**
(anchored children are arranged and emitted after their in-flow siblings),
**ADR-0104** (the scale is the caller's, `ui_scale` is the only calibration) and
**ADR-0088** (the style target is Windows 10). Everything you need is restated here.

## Goal

**A panel with a label and a button, drawn from the layout tree, that responds to the
mouse.** After this card the GUI is real: something on screen changes because a
person moved a mouse and clicked.

Four things, and they arrive together because none of them is testable alone:

1. **Emission** — layout's rectangles become element records.
2. **Identity** — a widget can be found again next frame, so it can have state.
3. **Measure** — `text` says how big a string is, so a label has a natural size.
4. **Three widgets** — panel, label, button, with hover and pressed.

**Hardcoded colours.** The theme is card 036 and this card must not anticipate it:
grey, a lighter grey, an accent, written as constants in one place with a comment
saying 036 replaces them.

## 1. Emission

- **Walk the laid-out tree after `voe_ui_frame_end` and submit one element per
  visible node.** `voe_ui_node_rect` gives the rectangle; `voe_render_element`'s
  `bounds` takes it.
- **It is a copy, not a conversion, and that is the point.** ADR-0099 put `ui`'s
  space and the element record's space in the same place — Y down from the top-left —
  precisely so this step has no arithmetic in it. 033's report confirms it:
  *"`voe_ui_rect.min` is the top-left corner, so min is `bounds.xy` and size is
  `bounds.zw` exactly."* **If you find yourself writing a subtraction here, stop and
  report** — something has drifted and it is cheaper to find out now.
- **Emit in the order layout arranged, not in tree order.** Under ADR-0092 submission
  order is paint order, and under ADR-0102 anchored children are arranged after their
  in-flow siblings so that they paint over them. Card 041 may not have landed;
  either way, **take the order from layout and do not re-derive it.**
- **Clipping**: every record carries a clip rectangle and a zeroed one clips
  everything away, so **every element this card emits gets a real clip rectangle** —
  the surface, or its parent's rectangle if you can argue for it. A scroll area is
  035 and it will narrow them; this card must not leave them zeroed.
- **Where the records go is the caller's**: `ui` produces them, `dev` submits them
  and draws them. `ui` still names only `render`, `text`, `math` and `base`, and it
  does not open a frame or issue a draw.

## 2. Identity — D-139, the part that reliably goes wrong

**Two widgets sharing a key silently share state**, which reads as a bug in the
widget rather than at the call site. That is why this is decided deliberately here
and not discovered later.

**The recommendation, and you are asked to argue it out loud rather than take it on
faith** — the 038 and 033 precedent:

- **A key is the parent's key mixed with what the caller supplies at that call
  site**: a string for a named widget, an index for a widget in a loop. A hashed
  path, not a call-site line number — `__LINE__` is the same for every iteration of a
  loop, which is exactly the case that matters.
- **It is stable while the tree's shape is stable**, which is what immediate mode
  needs and all it needs: the same calls in the same order give the same keys next
  frame.
- **Detect duplicates and say so.** Keep this frame's keys and refuse a collision
  loudly — a returned refusal or an assert, your call with a reason. **The register
  says the failure mode is silent sharing; the whole value of deciding this here is
  turning that into a noise.** A test that makes two widgets collide and proves it is
  caught is the most valuable test on this card.
- Say in your report what you chose, what the hash is, and what a collision does.

## 3. Measure, and the text scale

- **`text` gains a measure call**: a string's size in ems, or in whatever unit lets
  the caller multiply by its own millimetres-per-em. **`text` still knows nothing
  about elements or layout** — 031 made the per-character metrics public and this is
  the whole-string convenience over them, in the same folder, with the same
  ignorance.
- **ADR-0090: measured from the font in hand, never hard-coded.** No table of
  advances, no assumed line height.
- **The text scale, decided by the principal 2026-09-09**: a label's natural size is
  **the measured size multiplied by a `text_scale`**, a number beside `ui_scale`,
  default 1.0, and **the multiplication happens where the string is measured** —
  before the number becomes a natural size layout can see.
  - **So a label at 1.5 takes half again as much room and the row grows around it.
    Gaps and padding do not change.** One unit system throughout; nothing clips
    because layout reflowed.
  - It is *not* a second scale mode and there is no second coordinate space: the
    surface has one scale (ADR-0104) and this is a multiplier inside it.
  - It composes with ADR-0091's push/pop UI scale for a subtree; say in the header
    how the two combine, because a reader will ask.
  - **Test it**: the same label at 1.0 and 1.5 gives a row whose height grew and
    whose gaps did not.

## 4. The widgets

- **Panel** — a container with a background: a rectangle emitted behind its children,
  semitransparent if a caller says so (straight linear RGBA, premultiplied once by
  the shader — do not premultiply in `ui`).
  - **A fully transparent panel emits nothing at all.** An alpha of nought costs an
    element record, an instance and a blend to draw nothing, and *a transparent panel
    with padding* is a real use — the principal's own example is a full-screen one
    used as a television safe area, where older sets cut the edges off. Skip the
    emission and say so in the header.
  - **And note in the header that such a caller may not need a panel at all**: a
    plain `voe_ui_column_begin` with padding emits nothing already and is the same
    thing with fewer words. The panel widget is for when you want the background.
- **Label** — a string, its natural size measured as above, emitted as glyph elements
  through 031's kind. Its rectangle comes from layout like anything else's.
- **Button** — a rectangle, a label centred in it, and **three visual states**:
  normal, hovered, pressed.

### Hit testing

- **`ui` is *given* the pointer**: a two-dimensional point in the surface's own
  millimetres, plus the button states, as plain values every frame (ADR-0093).
  **`ui` does not name `platform`** and the tests prove it by running with no window
  system present.
- **The point arrives in the space layout already works in** — ADR-0099 again — so a
  hit test is a rectangle comparison and nothing more.
- **Hit test against the arranged rectangles, after layout**, which means a click is
  tested against *this* frame's positions.

### Click semantics, and they are decided

**A button fires on release inside its rectangle, and dragging out of it before
releasing cancels.** That is what Windows 10 does and ADR-0088 makes the style target
binding. It follows that this card needs to know *which* widget is being held.

### The one piece of state this card keeps

- **Two ids in the context: the hovered one and the held one.** Not a keyed table —
  that is D-141 and it arrives with 035's scroll offsets, which are the first
  *values* anybody needs to remember. Two ids are what press-and-release needs and no
  more.
- **They are cleared when the button comes up**, and a held widget that is not called
  this frame is no longer held. Say what you did about that case; it is how an
  immediate-mode GUI leaks a stuck button.

## What must not change

State in your report that you checked each of these:

- **`ui` names only `render`, `text`, `math`, `base`** — no `platform`, no `ecs`, no
  `3d`, no `scene`. The tests run with no window system. **Note that this card is
  where two of those four become real edges for the first time**: the folder is built
  today against `math` and `base` only, and emission adds `render` while measuring
  adds `text`. Both are permitted and neither is new policy — say in your report that
  you added them, so the day the map is read it is not a surprise.
- **No theme, no colour file, no roles.** Card 036, and 038's parser already landed
  for it. Constants here.
- **No text input, no caret, no selection.** Outside the first pass by the
  principal's own decision, and it is where GUI projects stall.
- **No scroll area, no clipping narrower than the surface, no checkbox, no slider.**
  035.
- **No second Y flip** (ADR-0033 point 6, ADR-0099). Emission is a copy.
- **The record's size and fields.** 030 and 031 own them; this card is a caller.
- **The two passes stay deferred.** Nothing is laid out during the calls.

## Where this card is likely to go wrong

- **Hit testing before layout.** The rectangles do not exist until `frame_end`; a
  test written during the calls uses last frame's geometry, which is the defect
  ADR-0091 exists to avoid.
- **Firing a button on press** because it is one line shorter. It is the wrong
  behaviour for the style target and it makes the held id pointless.
- **A stuck held id** when the widget that was pressed is not called next frame — a
  panel closed while a button is down. Decide it and test it.
- **Colours drifting towards a theme.** A `struct` of named roles here is card 036
  arriving early and badly; constants with a comment.
- **Emitting the label before the panel**, so the background covers the text. Paint
  order is submission order and there is no depth to save you.
- **Premultiplying in `ui`.** The shader does it once; doing it twice gives an
  interface that reads as badly chosen colours rather than as a bug.
- **A label measured with a hard-coded line height** because the font's number was
  inconvenient. ADR-0090 forbids it by name.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **`ui` tests, plain C, no window system** — the pointer is a value, so all of this
  is testable without a window. **All but the text-scale check need no graphics card
  either**; that one takes a headless device, and skips with a message naming itself
  if there is no driver (see the amendment at the top, ADR-0106):
  - a synthetic pointer over a button: hovered true, held false;
  - press inside, release inside: fires exactly once;
  - press inside, move out, release: does not fire, and the held id is cleared;
  - press inside, move out, move back, release: fires — that is what *cancels*
    means and it is the case people get wrong;
  - two widgets given the same key: the collision is detected and reported, not
    shared silently;
  - the same label at `text_scale` 1.0 and 1.5: the row grew, the gaps did not;
  - a button not called this frame while held: no stuck state.
- **An emission test**: a known tree produces a known list of element records — count,
  order, and that a panel's background comes before its children.
- **A screenshot**: a semitransparent panel, a label, and two buttons in their three
  states, with the draw count printed beside it. **The draw count is the claim** — a
  whole interface plus its text is one draw command, and card 040 may have put that
  number on screen for you.
- Windows is the principal's, and this is the first card where he can *click* on
  something. Say what you think he should try.

## Report when this lands

- The identity scheme, the hash, and what a collision does.
- The measure call's shape, and how `text_scale` and the push/pop UI scale combine.
- **What the headless device cost the text-scale check** — how long it adds to the
  run, and whether the skip path was exercised on purpose or only reasoned about.
  ADR-0105 is binding on this card's own verification too.
- The emission order, stated precisely enough that 035 and 041 can rely on it.
- What the two ids do in every awkward case you found.
- The element count and the draw count for the screenshot's interface.
- What you deliberately did not build, so the next person knows what 035 owes.


## Notes — done 2026-09-10, verified on Linux

`cmake -P check.cmake` exits zero, every step: 39 tests (ui 2, up from 1),
analyser clean over 105 files, all twelve folders configure standalone. Linux
only; Windows is unchecked and what it turns up is a new card.

### The identity scheme, the hash, and what a collision does

A key is **FNV-1a over a path**: the enclosing keyed widget's key as the seed,
then the `name` bytes, then the four bytes of `index`. The root's seed is the
FNV basis. Only panels and buttons are keyed, so a plain row or column between
two of them is stepped over and wrapping a button in a row does not rename it.
Every widget takes `name` and `index` as one shape — a named widget passes its
name and nought, one in a loop passes one name and the counter — so `__LINE__`
never comes into it and a list of buttons is a list of keys.

FNV-1a and not something stronger because the failure being guarded against is
two call sites written with the same name, not an adversary choosing strings.

**A collision refuses the frame.** Not an assert: the register's failure mode is
silent sharing, a refusal is what a test can observe, and the folder already had
this exact channel for a frame that wanted too many nodes. The duplicate is
caught at the call that made it — an open-addressed set of this frame's keys,
sized to twice the node capacity so the probe always ends — named once on stderr
with its name and index, and `voe_ui_frame_end` comes back false. The next frame
lays out normally. `tests/widgets.c` proves both halves: the duplicate is
refused, and the same name under two different panels is two widgets.

### The measure call, and how the two scales combine

    typedef struct { voe_math_float2 size; float baseline; } voe_text_measure;
    voe_text_measure voe_text_font_measure(const voe_text_font *font,
                                           const char *utf8);

In ems. `size` is deliberately the same box `voe_text_block.size` reports — the
widest line's advance, one line height per line — so the two text paths cannot
disagree about a string. `baseline` is the font's own ascender and is what a
surface needs that a pen does not: it says how far below the top edge of a
rectangle the first baseline sits. Without it, `ui` would have had to assume a
line height, which ADR-0090 forbids by name.

`text_scale` multiplies the measurement **where the string is measured**, in
`voe_ui_label`, before the number becomes a natural size. So layout sees
millimetres and does ordinary layout; a label at 1.5 grows and the gap and the
padding beside it do not. It composes with the surface's own scale (ADR-0104) by
plain multiplication and in that order — the surface scale decides how big a
millimetre is, `text_scale` decides how many millimetres a letter is worth — and
a subtree push/pop when ADR-0091's arrives composes the same way, because all
three are multipliers on one unit and there is no conversion between them.
`voe_ui_text_scale_set` is the knob; the base size is one constant, `TEXT_EM`,
beside the colours, and card 036 owns both.

### The emission order, precisely, for 035 and 041

Records come out in **`voe_ui_paint_order`'s order, and widgets.c never derives
it**. That accessor lives in layout.c beside `arrange` and today returns its
argument, because the node array is call order, arrange sweeps it forwards, and
a parent therefore lands before every one of its children.

So, as a contract 035 and 041 may rely on:

- a node's own records precede every one of its children's — a panel's
  background is behind what is in it, a button's behind its label;
- siblings emit in call order;
- a panel emits one record, a button one, a label one per character that draws
  and none for a space, in reading order;
- **041 changes `voe_ui_paint_order` and changes nothing in widgets.c.**

Every record's `clip` is **its own bounds** — never zeroed, and it clips nothing.
That is what 035 narrows. A zeroed clip draws an empty interface and is the trap
the card named.

Emission is a copy: `min` is `bounds.xy`, `size` is `bounds.zw`, and there is not
one subtraction between `voe_ui_node_rect` and `voe_render_element.bounds`. The
one minus sign in the folder is inside a label, turning a font's +Y-up box into
the surface's Y-down one, and it is the same expression `dev/src/elements.c`
already uses.

### The two ids, in every awkward case

`hovered` is recomputed every frame; `held` is the only thing that survives one.

- **Press arms on the edge only.** Arriving over a button with the mouse already
  down arms nothing, so a drag begun elsewhere does not press what it ends over.
- **A drag off un-hovers and keeps held.** Release then finds them disagreeing
  and fires nothing. Drag back on and it fires.
- **Release always lets go**, fired or not.
- **Held and not called this frame → dropped**, checked before the press and the
  release so a release cannot fire something that no longer exists. This is the
  stuck-button leak and it has its own test.
- **A refused frame holds nothing** — it has no rectangles, so it cannot honestly
  claim anything is hovered.
- **Releasing over a different button fires neither.**
- `over = false` (pointer gone, or locked for mouse-look) ends the hover.
- `fired` is true for exactly one frame, and there is a test for the "exactly".

The last widget in paint order wins the pointer, because the last painted is the
one in front.

### The screenshot, and its two numbers

**29 element records, ONE draw command.** A semitransparent panel (alpha 0.72),
the heading "Interface", and two buttons with labels — the whole thing including
every letter goes out as one `voe_render_frame_draw_elements`. In the running
program the console says:

    interface  29 element records for a panel, a heading and two buttons, in ONE draw command
    draws      33 commands for 123 element records; the walk was 31 of them

The interface moved the frame's command count by exactly one.

The picture is `~/voe3d-scratch/capture/ui-states.png` (throwaway, outside this
repository, as spikes are): four tiles down the page — both buttons normal, the
first hovered, the first held in the accent colour, and then the release, where
the label has become "Clicked 1". The same `voe_dev_interface_draw` and the same
transform the window uses, on a headless device with a synthetic pointer,
because there is no screenshot tool on this machine.

### What the headless device cost, and the skip

The whole `ui` test program is **0.28 s**, device and font included; the device
and the atlas are the bulk of it and the fourteen click, identity and emission
cases before them are microseconds. Nothing worth avoiding.

**The skip path was exercised on purpose, not reasoned about.** Run with a
`LD_LIBRARY_PATH` holding a `libvulkan.so.1` that is not a library, the test
prints and exits zero:

    render: libvulkan.so.1 is not installed on this machine
    skip: no graphics driver — the text-size and label-emission checks did not run

Note what it does **not** skip on: a loader that is present but has no ICD comes
back `REFUSED`, and that fails rather than skips — a driver that was there and
said no is worth stopping over. Same rule `3d/tests/panel.c` already uses.

### What was not built, so 035 and 036 know what they own

- **No theme.** Four colours and one text size as constants in `ui/src/widgets.c`,
  each with a comment naming 036.
- **No scroll area, no clip narrower than a widget's own bounds.** 035.
- **No keyed value table.** `ui` keeps two ids and nothing else; D-141 and 035's
  scroll offsets are the first values anyone needs remembered. The dev exhibit's
  click counter lives in `dev`, which is what that decision looks like from the
  caller's side.
- **No text input, no caret, no selection. No checkbox, no slider.**
- **No push/pop subtree scale** — ADR-0091 has it, nothing has asked for it, and
  `widgets.h` says how it will compose when it arrives.
- **No anchoring.** The exhibit pushes its panel down the page with a spacer box,
  which is the line 041 deletes.

### Three things worth the tech lead's eye

1. **`ui` gained its `render` and `text` edges here**, as the card said it would:
   `ui/CMakeLists.txt` is now `DEPENDS render text math base`. Both were already
   in `cmake/voe.cmake`'s row for `ui`; nothing about the map changed.
2. **`dev` gained a `ui` edge**, and that one needed a line in `cmake/voe.cmake` —
   `ui` was not in dev's row because it had never been used from there. dev's row
   says in its own comment that anything that exists is fair game, so this is the
   row working rather than a policy change, but it is a change to the map and is
   flagged rather than buried.
3. **`text` gained a second public thing beyond the measure call**: the UTF-8
   decoder, moved from `src/utf8.h` to `include/text/utf8.h`. It is not
   discretionary. `voe_text_font_measure` decodes the string; if `ui` stepped over
   bytes instead, the two would disagree the moment a character took more than
   one byte — a label measured for two letters and drawn as four. `text`'s own
   comment in `dev/src/elements.c` already said "a caller that needs it is `ui`".

### What the principal should try on Windows

The interface is on the screen-filling surface, below the readout on the left.

1. **Move the mouse over "Clicked 0".** It should go from grey to a lighter grey
   the instant the pointer crosses the edge, and the other button must not move.
2. **Press and hold on it.** Accent blue. Let go: the label counts up by one.
3. **Press it, drag off it, let go.** Nothing happens and the count does not move.
   This is the one that is wrong in most first attempts.
4. **Press it, drag off, drag back on, let go.** It fires. The button should go
   back to the accent colour the moment the pointer comes back on.
5. **Press it and drag the mouse right out of the window.** Windows takes the
   capture away and reports the button up, so the press is cancelled — nothing
   should fire and nothing should stay lit when the pointer comes back.
6. **Resize the window.** The interface keeps its size and its shape and the
   buttons stay clickable where they are drawn — the mouse's pixels and the
   surface's millimetres go through the same one division.
7. **Tab into fly mode.** The pointer is locked, so nothing is hovered; Escape
   hands it back and hovering works again on the next movement.
8. Change `VOE_DEV_UI_SCALE` in `dev/src/surface.h` to 2.0 and rebuild: the whole
   interface doubles and stays clickable.

### DEVIATION and BLOCKED markers

None. Nothing was guessed at: the one thing the card could not answer — how a
`ui` test gets a font with no graphics card — was asked about rather than
assumed, and the card's amendment answered it.
