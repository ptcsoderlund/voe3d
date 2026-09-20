# 16 — A container can be declared to take the pointer
folder: ui
decisions: 0168, 0199

## Change
An overlay is solid: nothing under it hovers or takes a click through the gaps between its items (ADR-0199).
`ui` has no way to say that today — a panel is drawn but only a widget answers the pointer, so the cursor falls
between a list's rows onto whatever the list covers. This card declares the one field that says it and carries
it to the node; nothing reads it yet, and nothing behaves differently. Card 17 is what makes it mean something.

`ui/include/ui/layout.h`:

- `voe_ui_container` gains one field, last, after `scroll`:

		// Whether the pointer stops at this container. False — which is
		// what a container that never mentions it is — and the pointer
		// reaches whatever is painted underneath, as it always did. See
		// A CONTAINER MAY TAKE THE POINTER at the top of this header.
		bool blocks_pointer;

- One prose paragraph at the end of the section `---- A CHILD THAT LEAVES THE FLOW ----`, after the paragraph
  AN ANCHORED CHILD IS A CONTAINER LIKE ANY OTHER and before the `---- WHAT IT WANTED, BESIDE WHERE IT WENT
  ----` heading, under its own capitalised opening: A CONTAINER MAY TAKE THE POINTER, AND AN OVERLAY IS WHY IT
  CAN. It says that a container declared `blocks_pointer` stops the pointer at its own VISIBLE rectangle:
  nothing painted before it — which is everything it is drawn over — is hovered, armed, pressed or fired
  through it, while its own children, painted after it, answer the pointer exactly as they always did. That
  this is what makes an open overlay solid (ADR-0199): the gaps between a list's rows and the padding at its
  edges belong to the list, so a cursor resting between two rows cannot light up the field the list covers. That
  it is the visible rectangle and not the node's own, so a blocker its clipping ancestors cut in half blocks
  only the half that is left and one wholly clipped blocks nothing — the same rule that already decides what
  can be hit at all. That it says nothing about the wheel: `voe_ui_pointer.scroll` still starts at the innermost
  scroll area under the pointer, wherever a blocker is. And that this header declares the field and never reads
  it — layout carries it as it carries `wrap`, and what it means is the hit test's, which is why the rule above
  is stated here and enforced in one place beside the hit test.

`ui/src/context.h`: `struct voe_ui_node_record` gains it among the fields the caller declared, after `scroll`:

		// Whether the pointer stops at this container, as the caller
		// declared it (ui/layout.h). layout.c writes it and never reads
		// it: what it means is button.c's hit test, exactly as `wrap`
		// means something only to the sweeps. False on a leaf, which
		// takes a sizing and not a container.
		bool blocks_pointer;

`ui/src/layout.c`: the one place a container's declaration lands on its node — where `n->wrap =
container.wrap;` sits, around line 1132 — copies it too: `n->blocks_pointer = container.blocks_pointer;`. No
assert: every combination of it is meaningful.

## Done when
`checks.sh --folder ui` exits 0 and `cmake --build --preset debug` builds the whole tree. `grep -rn
blocks_pointer ui` prints the declaration, the node record's field and the one copy, and nothing else — no
reader yet. Every `ui` test passes unchanged, because nothing this card touches is read.
