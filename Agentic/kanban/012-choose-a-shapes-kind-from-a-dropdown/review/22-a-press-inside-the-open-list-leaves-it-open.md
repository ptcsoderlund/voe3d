# 22 — A press inside the open list leaves it open
folder: editor
decisions: 0168, 0195, 0198, 0199, 0200

## Change
A capped list has a scrollbar, and pressing it must not be the press on nothing that closes the list. The rule
becomes what an overlay's rule always was: a press outside the outline closes it, a press anywhere inside it —
the bar, the gaps between the rows, the padding at the edges — is the list's and leaves it open (ADR-0199). The
press needs a position, which this file is not given today.

`editor/src/inspector_edit.h`:

- `voe_editor_inspector_buttons_read` gains the pointer's place beside the button it already takes:

		void voe_editor_inspector_buttons_read(voe_editor_inspector *inspector,
						       const voe_ui_context *ui,
						       struct voe_editor_scene *scene,
						       bool down, voe_math_float2 at);

  with `<math/float2.h>` among the includes. Its comment says `at` is where the pointer is this frame, in the
  surface's millimetres, and what it is for: a press is only a press on nothing when it landed outside the open
  list.
- The paragraph about a fired dropdown control gains the whole of what this file now does with an open list, in
  its own voice: a row that fired is submitted at once and closes it; a press that fired no row and no dropdown
  control closes it only when it landed outside the list's visible rectangle, because everything inside that
  outline belongs to the list (ADR-0199) and the bar of a capped one is in there; a frame in which the list drew
  no rows closes it; and where it sits is worked out from the button's rectangle and the room the panel's scroll
  area leaves round it — below it when it fits there, above it when it fits there instead, and on the roomier
  side capped to that room and scrolling when it fits neither (ADR-0200) — set through scene.h every frame,
  because an overlay is placed where it fits each frame and never once when it opened. Say that the arithmetic is
  in the Inspector's content column's space and therefore says nothing about scrolling.

`editor/src/inspector_edit.c`, in `voe_editor_inspector_buttons_read`: the press that closes the list — card
14's step 3 — closes it only when `inspector->list` is `VOE_UI_NODE_NONE`, meaning no list was drawn at all, or
`at` is outside `voe_ui_node_visible(ui, inspector->list)`. The two comparisons per axis are written as a small
static above the function, or the way interface.c already writes the picker's press-outside test; nothing else
about the step changes, and the press edge is still read before `inspector->pointer_was_down` is written.

`editor/src/interface.c`: the one call passes `root->pointer.at` beside `root->pointer.down`.

## Done when
The coder: `checks.sh --folder editor` exits 0 and `cmake --build --preset debug` builds the whole tree.

The human, at a running `voe_editor`: opens a kind dropdown and presses in a gap between two rows and in the
padding at the list's edge — the list stays open both times and nothing under it reacts. Makes the panel short
enough that the list is capped, then drags its scrollbar: the rows move and the list stays open. Presses
somewhere else in the editor: it closes, the kind unchanged, as does Escape.
