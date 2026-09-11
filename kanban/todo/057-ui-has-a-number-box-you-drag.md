# 057 — `ui` has a number box you drag sideways

claimed-by: -
blocked-by: -
status: todo
decision: *The inspector edits by dragging first, and is shaped for typing* (ADR-0136) points 1–3 — horizontal drag changes the value, a fine modifier handed in as a value slows it, the widget hands back the new value rather than a distance, and a press and release without movement does nothing and is reserved for typing.

## Goal

`voe_ui_number_begin` and `voe_ui_number_action` exist in `ui/include/ui/widgets.h`, tested
with no window and no graphics card.

## Scope

**1. `ui/include/ui/widgets.h`.**

- `voe_ui_pointer` gains `bool fine`, true while the caller's fine modifier is held. Zero is
  *not fine*, so the designated initialisers already in `dev/src/interface.c` and the tests
  need no edit.
- A container like the button, closed by `voe_ui_end`, with what is composed inside it
  centred:

```c
voe_ui_node voe_ui_number_begin(voe_ui_context *ui, const char *name, uint32_t index,
				double value, double per_millimetre);

typedef struct {
	bool hovered;
	bool held;
	bool changed;  // this frame's drag moved the value
	double value;  // the value handed in, plus this frame's drag
} voe_ui_number_result;

voe_ui_number_result voe_ui_number_action(const voe_ui_context *ui, voe_ui_node number);
```

- The header says, in its own style: `ui` knows no field kinds — the caller gives the value
  and what one millimetre of horizontal drag is worth, and rounds for an integer; the label
  is composed in, as with a button; **the result is a value and not a distance, which is the
  contract a typed entry will keep**; **a press and release without movement does nothing,
  and nothing may be bound to it**; a number box never fires.

**2. Behaviour — `ui/src/widgets.c`, `ui/src/context.h`.**

- It is armed and let go exactly as a button is: a pointer already down when it arrives arms
  nothing; release always lets go; a held number box the frame did not build is let go.
- **Dead zone**: while held, nothing changes until the pointer is `VOE_UI_NUMBER_DEAD_ZONE`
  (1 mm) horizontally from where it was pressed. On the frame that crosses it the change is
  the distance beyond the dead zone; on every frame after, it is this frame's `at.x` minus
  the previous frame's. Either way it is multiplied by `per_millimetre`, and by
  `VOE_UI_NUMBER_FINE` (0.1) while `fine`.
- A drag keeps working with the pointer past the surface's edge — `platform` keeps reporting
  the pointer while a button is held.
- The context keeps the press position, the previous frame's `at.x` and whether the dead
  zone was crossed. That is interaction state beside `held`; no keyed table.
- It looks like a button: the same three colours and the same padding.

**3. Tests — `ui/tests/widgets.c`.**

- Pressed at x 0, next frame at x 10: the value moves by 9 × `per_millimetre`; a further frame
  at x 12 moves it by 2 more.
- Movement within the dead zone changes nothing; with `fine`, a tenth.
- A press and release with no movement: `changed` false on every frame.
- A drag past the surface's right edge keeps changing the value.
- A pointer already down when it arrives arms nothing; a number box not built is let go.
- A number box and a button with the same key refuse the frame.

**4. `ui/ui.md`** — `widgets.h`'s entry names the number box. The header's *WHAT IS NOT HERE*
still says there is no text input, caret or selection.

## What must not change

- The panel, the label, the button, and the press-and-release rules they follow.
- `ui` names no `platform`, `ecs`, `scene` or `3d`.
- No text input, no caret, no typing: reserved, not built.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R ui` passes.
- `grep -rn 'platform/\|ecs/\|scene/' ui/include ui/src` returns nothing.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A test that drags a number with no window anywhere, and a click on it that does nothing on
purpose.

## Notes
