# 0192 — Typing into a value and picking a colour are `ui`'s
date: 2026-09-19
by: planner

## Decision
For spec 010. Both are `ui` widgets any panel can use; the editor only places them.
- **One keyboard focus, and `ui` holds its text.** A field and a number box that is being typed into are the
  typeable widgets. While one has the focus, `ui` holds its text in the context, seeded from the caller's value
  when the focus arrives, and ignores the caller's value until the focus leaves. At most one widget has it.
- **The focus arrives with the whole text selected**: the first typed text replaces it, Backspace empties it.
- **Enter, Tab, or a press elsewhere commits. Escape cancels** and the caller's own value stands. Tab moves the
  focus to the next typeable widget made in the frame, in call order, wrapping to the first. `voe_ui_keyboard`
  gains `escape` and `tab`. `voe_ui_typing(ui)` says whether a widget held the focus at the last frame's end, so
  a program can keep Escape, Delete and its shortcuts from a person who is typing.
- **A number box opens for typing on a press and release inside its dead zone.** It opens with the value as
  `%.6g`. A commit with the text unchanged changes nothing. A commit with text that is not one finite number
  (`strtod`, blanks allowed around it) is refused. On Enter or Tab the box stays open and says "not a number". On a
  press elsewhere it closes as Escape does.
- **The colour picker is a panel the caller places**, not a popup `ui` opens. It takes and hands back a linear
  RGB colour. It shows a saturation/value square and a hue strip in HSV of sRGB, drawn as grids of solid element
  cells, and a hex field (`#RRGGBB` or `RRGGBB`, any case). It reports whether a press began outside it, and the
  caller decides that this closes it. A swatch is a solid box of a given colour, which a caller puts inside a
  button.

## Reasoning
Spec 010 asks that the picker and the click-to-type box be usable by any later panel, so neither can live in the
editor. `ui`'s number box already reserved the click for typing (ui/widgets.h) and hands back a
value, not a distance, so a typed value arrives through the same result. Holding the text in the context is what
lets a number box, which is handed a double and not a string, be typed into at all. It also makes Escape
possible without every caller keeping a copy. A refused Enter that stays open is how "the box says so" is seen.
A refused press elsewhere closes, because the press has already gone to something else. The element path draws
only solid rectangles and glyphs, so a gradient is a grid of cells. A texture would need a startup upload `ui`
cannot make. A picker the caller places avoids a clip `ui` does not have: the Inspector's scroll area would clip
a popup anchored to the swatch.

## Replaces
The number box header's "a press and release without movement does nothing, and nothing may be bound to it".
