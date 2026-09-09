# 034 — the first widgets: panel, label, button

status: todo
claimed-by: -
blocked-by: -

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
  `3d`, no `scene`. The tests run with no window system.
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
- **`ui` tests, plain C, no graphics card** — the pointer is a value, so all of this
  is testable without a window:
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
- The emission order, stated precisely enough that 035 and 041 can rely on it.
- What the two ids do in every awkward case you found.
- The element count and the draw count for the screenshot's interface.
- What you deliberately did not build, so the next person knows what 035 owes.
