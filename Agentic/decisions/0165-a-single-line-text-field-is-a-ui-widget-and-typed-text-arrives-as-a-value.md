# 0165 A single-line text field is a `ui` widget, and typed text arrives as a value

Status: accepted
Date: 2026-09-16

The editor's file browser has to take a typed folder name, and `ui` has never had text input: no
caret, no focus, and a number box that is dragged rather than typed into. The box could have been
drawn inside the editor out of a panel and a label, which would put the tree's only text input in a
program no game links. The sponsor decided the editor uses the same field a game would.

## Decision

`ui` gains a single-line text field, and `ui` still reads no keyboard. Typed text is handed over
per frame as a value — `voe_ui_keyboard_set` with the UTF-8 typed since the last frame and the two
edit keys the field understands — exactly as the pointer is handed over (ADR-0093), so the folder
still names no `platform` and every case is testable with no window system.

The field takes its text as a parameter rather than composing a label in as a button and a number
box do, because the caret's place is measured from that text and the widget cannot ask a caller
what it drew. What comes back after `voe_ui_frame_end` is the edited text as a value, the way a
number box hands back a value: `ui` never writes the caller's buffer.

Its scope is what a name box needs and no more (rule 10): appending typed code points, Backspace,
Enter reported, a caret at the end of the text, and one focus that a press moves.

## Rejected

- An editor-private name box — the only text input in the tree would sit in a program no game
  links, and the first game to want one would write a second. This is the sponsor's dogfooding.
- `ui` reading the keyboard itself — it would have to name `platform`, which reverses this folder's
  arrow and ends headless testing, the same reason the pointer is given and not asked for.
- A caller-owned buffer the field writes in place — a widget mutating another owner's memory inside
  `voe_ui_frame_end`, with no way to say when it happened. The value contract already exists here.
- A movable caret, selection, clipboard, multiple lines, and typing into a number box — nothing
  calls for any of them. The number box's click stays reserved for the day one does.

## Consequences

- `ui`'s "no text input, no caret and no focus" is no longer true and its header says what is and
  is not there instead.
- Focus is one key in the context, held between frames like `held`. A field that is not called in a
  frame is not focused at that frame's end, as a scroll area that is not called is forgotten.
- The edited text lives in one fixed buffer in the context, because only the focused field can be
  edited in a frame. It is valid until the next frame begins.
- Typing into a number box, when it arrives, is this field's editing under that widget's click.
