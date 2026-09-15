# 057 — `ui` has a number box you drag sideways

claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -
status: review
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

**Verified on Linux, 2026-09-12.** `cmake -P check.cmake` green end to end — 41 tests
passed, analyser clean over 110 files, every folder still configures standalone.
`ctest -R ui` 2/2. `grep -rn 'platform/\|ecs/\|scene/' ui/include ui/src` returns nothing.
From the planning root `bash tools/hot.sh` reports all hot files under their ceilings;
`ui/ui.md` is 71/120.

Windows was not checked — one machine, one platform (2026-09-04). Nothing here is
platform-specific: the pointer is a value handed in, and every one of the nine new cases
runs with no window and no graphics card.

No `BLOCKED:` and no `DEVIATION:` markers. Six files changed, all in `ui` plus this card.
`dev/src/interface.c` needed no edit, as the card said: `fine` is a new field at the end of
`voe_ui_pointer` and its designated initialisers still mean what they meant.

**The tests were checked against a deliberate break.** Dropping the dead zone from the first
change — `change = from_press` instead of `from_press - VOE_UI_NUMBER_DEAD_ZONE` — fails
three of them. The mutation was reverted before anything else was run.

**Two judgment calls, both narrow readings, and worth your eye:**

- **`over` does not end a drag.** The card wants a drag to keep working past the surface's
  edge, and `voe_ui_pointer`'s comment said `at` is ignored when `over` is false. The
  existing code already treated `over` as hover-only — the press and release edges are read
  regardless of it — so the number box follows that precedent: `over` governs hovering and
  arming, `down` governs the gesture. I corrected the comment on `over` to say so, since it
  now describes something the folder relies on.
- **The fine modifier slows the value and not the dead zone.** The card says the change is
  multiplied by `VOE_UI_NUMBER_FINE` and says nothing about the zone, so the zone is left at
  a millimetre in both. A fine drag that also had a tenth of the dead zone would begin
  *sooner* than an ordinary one, which is backwards; `a_fine_drag_moves_a_tenth_as_far`
  pins the reading either way.

**A number box never fires**, and that is enforced at the release rather than left to the
fact that `voe_ui_number_result` has no `fired` field: `resolve` will not set `fired` for a
held number box at all, so no key belonging to one can ever come back through
`voe_ui_button_action`.

**Suggestion, not done here:** `VOE_UI_NUMBER_DEAD_ZONE` and `VOE_UI_NUMBER_FINE` are public
macros because the tests need the numbers and a caller explaining the gesture might. When
card 036 brings a theme, both look like things it would rather own than a header would.
