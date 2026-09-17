# ui

Nested rows and columns of boxes in millimetres, a rectangle for every one of
them, and the first widgets on top: a panel, a label, a button that answers the
mouse, a number box you drag sideways to change a value, a single-line text
field, an image and a scroll area that remembers its offset. Not drawing — what
comes out is element records and the caller submits them — and not input
either: the pointer and the keyboard are values it is handed. Where the surface
sits in the world is one matrix and it is the caller's. It also turns an
authored theme — one colour, two scalars, a mode and a text size — into the
palette a widget draws with (`include/ui/theme.h`); reading a theme file into
those authored values is a different folder's job (`theme`, ADR-0168). Every
widget draws from the nearest theme in force — set on the context or pushed
over a subtree (`voe_ui_theme_set`, `voe_ui_theme_push`/`voe_ui_theme_pop`).

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
- `include/ui/theme.h` — `voe_ui_theme_inputs` (the five authored values),
  `voe_ui_theme` (the derived palette of roles), `voe_ui_theme_default_inputs`
  and `voe_ui_theme_derive`. Its header says why `accent` stays sRGB in the
  inputs and is linear in every derived role, why the derivation runs in OKLab
  and cannot fail, why a NULL font is allowed, why the accent's chroma is
  clamped harder in dark mode than in light, and why
  `VOE_UI_THEME_SCALAR_MIN`/`MAX` are public — `theme` has to refuse the same
  range this folder clamps to.
- `include/ui/widgets.h` — the theme mechanism (`voe_ui_theme_set`,
  `voe_ui_theme_push`, `voe_ui_theme_pop`), the panel (`voe_ui_surface`), the
  label (`voe_ui_text_role`, `voe_ui_label_role`), the button, the number box,
  a single-line text field (`voe_ui_field`, `voe_ui_field_focus`,
  `voe_ui_field_action`, `voe_ui_field_result`, `VOE_UI_FIELD_CAPACITY`), the
  image and the scroll area (`voe_ui_scroll_begin`, `voe_ui_scroll_axes`), the
  pointer and the keyboard (`voe_ui_keyboard`, `voe_ui_keyboard_set`) they are
  given, and this frame's element records read back. Its header says why the
  answer to a click arrives after the frame has ended rather than at the call,
  what a widget's key is made of and why it is a hashed path and not a line
  number, what two widgets sharing one does, why a button is composed rather
  than handed a string, why a NONE panel emits nothing, why a panel's and a
  button's hairline border is two element records and not one, why a widget
  reads the theme in force at the call that makes it and not again at
  emission, why an unmatched push refuses the frame while an unmatched pop
  asserts, and how the theme's `text_size` composes with the surface's own
  scale. On the number box it says why this folder knows no field kinds and
  takes a value and a rate instead, why what comes back is a value and not a
  distance and what that buys the typing that is not built yet, and why a
  press and release without movement is reserved rather than free. On the
  field it says why it composes its own label rather than taking one in, why
  the caret is measured from that label, why the edited text comes back as a
  value rather than the caller's own buffer being written into, that where the
  typed bytes came from is not this folder's business, and what a field costs
  in nodes and in element records. On the image it says what it is for, why it
  is sized as a box is, that it is one element, and that the texture's
  lifetime is the caller's. On the scroll area it says why the offset is
  remembered there and not in layout, that an area not called forgets, how a
  scroll passes outward, why it arrives in millimetres and lands next frame,
  and that the bar lies over the content.
- `src/context.h` — the tree and the context, shared by the folder's two source
  files. Its header says why there is one context and not two, which half owns
  which field, why the widget pass runs where it does, why a drag needs four
  fields beside `held` and no keyed table, why a held thumb needs no key of its
  own, what `focus` is beside `held`, and why every node the widget pass reads
  a theme from copies it once, at the call that made it, rather than looking it
  up again.
- `src/layout.c` — the tree, and the sweeps over it, one axis at a time. Its
  header says why the array being in call order makes every pass a flat loop
  with neither recursion nor a stack, what passes between the X pass and the Y
  pass, where a wrapping column has to revisit X, why a corrective sweep after
  both passes measures each axis again and re-clamps every offset once the wraps
  are decided, why a node's natural size is written by its parent rather than by
  itself, what a grow child contributes to a natural container and
  why that answer and not the two others, why a gap belongs to the run and not to
  a child, how an anchored child is a stronger exclusion than a grow one and why
  its axes are absolute, how paint order is worked out in three linear sweeps now
  that it is no longer the array's own order, where a scroll offset is clamped
  and how the clip is a fourth flat sweep, where the single subtraction of
  padding lives and why four numbers still go through two accessors, why there is
  no flip and no minus sign in front of a Y anywhere in it, and why the structs
  are declared next door.
- `src/oklab.h` / `src/oklab.c` — the OKLab conversion `theme.c` derives every
  lightness step in, internal to this folder. Its header says why it lives
  here and not in `math`, why L, a and b are their own struct rather than a
  reused `voe_math_float3`, and why conversion clamps rather than failing at
  the edge of the gamut.
- `src/theme.c` — the derivation. Its header says why every role but the
  accent is grey, why the ground/surface/raised/control ladder is five equal
  steps of `surface_separation`, why text and the border are stepped from
  `ground` rather than from `surface`, and what "stepped until it clears its
  surface" means as code.
- `src/widgets.c` — what a node means, what the pointer and the keyboard are
  doing to it, and the records that come out. Its header says why emission is a
  copy with no arithmetic in it and where the one sign that does appear comes
  from, why paint order is taken from layout rather than re-derived, why the
  hit test is after arrange and against the visible rectangle, how a record is
  clipped, what the two ids do in every awkward case including the stuck one,
  why the key is FNV-1a over a path, why a drag measures its dead zone from
  the press and its change from last frame, how the scroll table is
  rewritten each frame and where a scrollbar sits in paint order, why a
  field's focus follows the press itself rather than `held`/`fired`'s release,
  why its editing runs after resolve rather than inside it, which theme role
  every widget's colours are, why a field's focused state reads
  `surface_raised` rather than a fourth control colour, and how a panel's or a
  button's hairline border is drawn as two records rather than one.
- `tests/layout.c` — rectangles worked out by hand, one case per decision,
  wraps, clips and clamped scroll offsets among them. Its header names the
  answers it is pinning down rather than merely exercising, group by group and
  each one a decision that could have gone the other way, which two cases are
  about the machinery instead of the arithmetic, and why exactly one case
  reaches into `src/` — paint order is an order and no rectangle can show it.
  Needs no graphics card and no window system.
- `tests/theme.c` — a round trip through OKLab, both modes legible at both
  ends of both scalars, that only the accent moves when only the accent
  moves, that each scalar moves what it names and nothing else, and the
  dark/light chroma asymmetry. Its header says why a NULL font is safe
  everywhere here and how `grey_lightness` reads an OKLab lightness back out
  of a linear role colour by cube root. Needs no graphics card.
- `tests/widgets.c` — a press and a release in every order a hand can produce,
  a sideways drag in every order one can, a clipped button and label, a drag
  scrolled out of sight, a scroll area's remembering, passing on, bar, drag,
  page and refusal, a duplicate key, a known tree emitted as a known list with
  every panel's and button's border and fill in the right order, the nearest
  theme winning over the one above it and a pop restoring it, an unbalanced
  push refusing the frame, a label in ACCENT coming out in the accent, two
  images as two IMAGE records, and a field's focus, typing, Backspace, Enter
  and capacity. Its header says why the click cases are the ones that matter,
  why the collision case is the most valuable in the file, and why every case
  that measures a string — the theme's own text_size, a label's own emission,
  and a field, which always composes one — takes a headless device while the
  rest need no graphics card. Needs no window system.
