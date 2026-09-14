# 0136. The inspector edits by dragging first, and is shaped for typing

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-243**: *how a person changes a value in the inspector, and what `ui` gains
first.* ADR-0134 settled how an edit reaches a component; this settles how a person makes
one.

What is on disk, read the same day. **Typed text exists on neither platform, on purpose.**
`platform/src/window_wayland.c` reads evdev scancodes and ignores the keymap, because
turning a scancode into a character needs `xkbcommon`, *"a dependency this engine has not
asked for"*; `window_win32.c` ignores `WM_CHAR` for the same reason. ADR-0023: a
dependency is the principal's call. `platform/include/platform/input.h` says text needs an
ordered history rather than polled state, and that the card bringing it *replaces* that
decision. **A drag, by contrast, is already built end to end**: `ui` is handed the pointer
as a value (ADR-0093), keeps a held widget across frames, and `platform` keeps reporting
the pointer past the window's edge while a button is held. Shift and Control are keys the
engine already reads. `ui` has a panel, a label and a button — *"no text input, no caret
and no selection … no checkbox and no slider."*

The first described component is the transform: two `FLOAT3`s and a `QUAT` (card 048).
The identity component adds a `CHAR` name (ADR-0125).

## Options considered

### Option A — drag first, typing later
A number box in `ui` dragged sideways; typing arrives on its own later.

### Option B — typing first
Ordered text input in `platform`, a text field in `ui`. On Linux it forces the `xkbcommon`
decision, or an in-house keymap reader, before the first editor card.

### Option C — a read-only first inspector
Editing waits for a later widget card.

## Decision

**Option A, with the principal's amendment**: *"Drag first, but we should prepare for
typing. We are going to need input controls."* Preparing is taken to mean **shaping what is
built now so typing is added and nothing is reworked**, and **putting typing's own decision
on the queue now** — not building what nothing calls (rule 10).

1. **`ui` gains a number box you drag sideways.** A press on it and horizontal movement
   change its value; a fine modifier, handed in as a value like the pointer, slows it.
   `ui` still names no `platform` and knows no field kinds: the caller gives the current
   value and how much one millimetre of drag is worth.
2. **It hands back the new value, not a drag distance.** That is the contract a typed entry
   will satisfy too, so the inspector does not change when typing arrives.
3. **A press and release without movement does nothing, and is reserved for typing.** A
   small dead zone separates a click from a drag. Click-to-type, drag-to-change is where
   the same box goes when typing exists; nothing may bind that click to anything else.
4. **What the first inspector edits, by kind:**
   - integers and floats, and each component of `FLOAT2`/`3`/`4`: the number box;
   - `BOOL`: a button that toggles;
   - `QUAT`: **three rows showing angles in degrees, each dragging a rotation about that
     world axis onto the quaternion.** The edit is incremental, so it stays a rotation and
     no editor-side angle state is kept;
   - `ENUM`, `CHAR` arrays, `ENTITY` and `FLOAT4X4`: **shown and not editable.**
5. **While a drag is held the inspector submits a replace intent each frame**, one per
   entity per frame — the sizing the owning systems already assume (ADR-0134).
6. **Typing's decision is on the hot queue now, waiting on the principal**: how typed text
   reaches the engine on Linux — `xkbcommon` or an in-house keymap reader — and the ordered
   text input that replaces polled state for text.
7. **The input-control inventory is live.** The question of how deep `ui`'s widget set goes
   returns to the hot queue on the principal's statement that input controls are needed.

## Blast radius

**Cheap.** The number box is one widget. Point 2's contract and point 3's reserved click
are what keep typing additive; breaking either later means reworking the inspector when the
text field lands. Reversibility: **cheap.**

## Consequences

- **One `ui` card before the editor card**, beside the `app` and `dev` cards ADR-0135 put
  there. No `platform` card.
- **The first editor cannot rename anything, and cannot set an exact value.** Dragging a
  position back to exactly zero is not possible. Accepted for the first card; it is the
  first thing typing fixes.
- **Displayed angles can jump** — the same rotation has more than one set of angles, and
  they are derived from the quaternion each frame. The edit is still correct because it is
  applied as a rotation, not by rewriting an angle.
- **Enumerations show their integer.** ADR-0123 authors an enumeration by name, and card
  048's field record carries no value names, so the inspector has none to show.
- **Float drift will slowly de-normalise a dragged quaternion.** Tiny per frame, and exactly
  what D-242 (what a drain does with a value that breaks its invariant) exists to answer.

## Rejected options and why

**B — typing first.** It forces the project's first dependency decision, or a large
in-house keymap reader, and replaces the input model, all to satisfy the first editor
card. The principal wants typing; this ADR puts its decision in front of him now instead
of letting a card force it.

**C — read-only first.** Short of what the principal asked for: an inspector you can edit.

## Questions this opens

- **D-245** — how typed text reaches the engine: `xkbcommon` or an in-house keymap reader
  on Linux, `WM_CHAR` on Windows, and ordered text input replacing polled state for text.
- **D-246** — what an enumeration's description carries so the inspector can show and pick
  its names.
