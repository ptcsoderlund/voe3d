# 0196 — State is drawn inverted, and a palette has an inverse and its ink
date: 2026-09-19
by: planner

## Decision
ADR-0194 removes the accent and says state is shown by inversion, weight, borders and lightness. The
palette's `accent` and `accent_ink` roles are replaced by `inverse` (a fill at `text_primary`'s lightness)
and `inverse_ink` (text at `ground`'s lightness), both in the theme's one hue. What was drawn in the accent
is drawn inverted: a held button, a number box being dragged, a held scrollbar thumb, a field's selected
text, and a new `voe_ui_choice_begin` (a button with a `selected` flag, which the editor's Scene rows use)
while selected. Every label made inside a control drawn inverted draws in `inverse_ink`, whatever role it
asked for, so the caller composes a plain label and the control decides its ink. Hovered stays the
`control_hovered` step. `VOE_UI_TEXT_ROLE_ACCENT` goes. Every role keeps the hue's chroma (clamped harder in
dark mode, ADR-0171) and shrinks only chroma where a lightness would leave the gamut, by bisection in
`ui/src/theme.c`; a grey hue gives zero chroma everywhere. The inputs field is `hue`, sRGB, its lightness
ignored.

A slider is a `ui` widget composed of public calls: a number box holding a fixed-width track with an
anchored thumb, its value clamped to a range the caller gives. It takes the number box's drag, typing and
inversion as they are.

## Reasoning
Inversion keeps the contrast of text on ground by construction: the fill is the text lightness and the ink
the ground's, so the legibility the derivation already proves covers the inverted pair. A heavier
background alone was rejected because at the high end of `surface_separation` it runs into the text
lightness and a label on it stops reading. Choosing the ink in the control rather than in the caller's label
call was preferred because a caller cannot know on the frame it composes a button whether that button is
held. A slider as its own widget kind with a context record, like the colour picker, was rejected as more
files and more state for what a number box already does.

## Replaces
Nothing. Implements ADR-0194's "state is shown by inversion" for `ui`.
