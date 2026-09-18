# ui

Nested rows and columns of boxes in millimetres, a rectangle for every one of
them, and the first widgets on top: a panel, a label, a button that answers the
mouse, a number box you drag sideways to change a value, a single-line text
field, an image and a scroll area that remembers its offset. Not drawing — what
comes out is element records and the caller submits them — and not input
either: the pointer and the keyboard are values it is handed. Where the surface
sits in the world is one matrix and it is the caller's.

- `include` — the public headers, in `include/ui/`; each is listed below by path.
- `src` — the implementation: the tree and the sweeps that settle it, then what a node
  means once the pointer and the keyboard have been at it; each file is listed on `src/src.md`.
- `tests` — one plain C program per module, found by the build, neither needing a window
  system; each is listed on `tests/tests.md`.
- `include/ui/layout.h` — the context and its `voe_ui_capacities` of nodes,
  element records and scroll areas, the frame, and rows and columns and boxes
  between its begin and its end, any of which may clip what reaches past it
  and offset the content it holds (`voe_ui_overflow`,
  `voe_ui_container.scroll`), with a rectangle, the visible part of that
  rectangle, the offset layout settled on and a measured size read back
  through a handle (`voe_ui_node_visible`, `voe_ui_node_scroll`). Its header
  says why nothing is laid out until the frame ends and why that is what makes
  the first frame right, why a call returns a handle and not a size, why
  nothing survives a frame, that the space is millimetres with Y down from the
  panel's top-left corner because that is the space an element record is
  already in, why that is not a departure from the world being Y-up, that a
  column runs from the top down so it reads in call order, that START is left
  and top on either axis, which of the three sizings may be used where, that a
  child's own fixed size across the flow beats the container's FILL, that
  overflow is reported rather than shrunk unless a container asks to clip it,
  per absolute axis and nested clips intersecting, that a clip narrows what is
  seen and never where anything is, that a scroll offset moves a clipping
  container's content and layout clamps it while remembering none of it, and
  the three ways a frame can be refused. It says that X is laid out for the
  whole tree before Y and that nothing may need a height to know a width, and
  how a container that asks to wrap breaks its run into lines — and why a
  wrapping column overflows to the right instead of widening. It also says why
  padding is four numbers named by absolute side, why there is no margin and
  what to do instead, what an anchored child is and why its two axes are X and
  Y rather than the flow's two words, which way its offset moves it, that it
  is measured against its parent's content box and paints over its in-flow
  siblings, the trap that a fit-to-children parent holding only anchored
  children has no natural size at all, and what the measured size is for.
- `include/ui/widgets.h` — the panel, the label, the button, the number box, a
  single-line text field (`voe_ui_field`, `voe_ui_field_focus`,
  `voe_ui_field_action`, `voe_ui_field_result`, `VOE_UI_FIELD_CAPACITY`), the
  image and the scroll area (`voe_ui_scroll_begin`, `voe_ui_scroll_axes`), the
  pointer and the keyboard (`voe_ui_keyboard`, `voe_ui_keyboard_set`) they are
  given, and this frame's element records read back. Its header says why the
  answer to a click arrives after the frame has ended rather than at the call,
  what a widget's key is made of and why it is a hashed path and not a line
  number, what two widgets sharing one does, why a button is composed rather
  than handed a string, why a fully transparent panel emits nothing, and how
  the text scale composes with the surface's own. On the number box it says why
  this folder knows no field kinds and takes a value and a rate instead, why
  what comes back is a value and not a distance and what that buys the typing
  that is not built yet, and why a press and release without movement is
  reserved rather than free. On the field it says why it composes its own label
  rather than taking one in, why the caret is measured from that label, why the
  edited text comes back as a value rather than the caller's own buffer being
  written into, that where the typed bytes came from is not this folder's
  business, and what a field costs in nodes and in element records. On the
  image it says what it is for, why it is sized as a box is, that it is one
  element, and that the texture's lifetime is the caller's. On the scroll area
  it says why the offset is remembered there and not in layout, that an area
  not called forgets, how a scroll passes outward, why it arrives in
  millimetres and lands next frame, and that the bar lies over the content.
