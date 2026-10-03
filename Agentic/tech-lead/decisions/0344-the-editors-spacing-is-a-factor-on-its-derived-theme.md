# 0344 — The editor's spacing is a factor on its derived theme
date: 2026-10-03
by: planner

## Decision
For 054 (0306): `voe_ui_theme` gains `spacing`, a multiplier on the millimetres `ui`'s widgets put
round their content (a button's, number box's and field's pad, the number box's refused gap, the
colour picker's pad and gap). `voe_ui_theme_derive` sets it to 1, it is not authored and not in
`voe_ui_theme_inputs`, so a game and `dev` keep their sizes. The editor names two constants in
`editor/src/themes.h`, `VOE_EDITOR_TEXT_BASE` 0.8 and `VOE_EDITOR_SPACING` 0.65: every editor theme
is derived at `text_size * VOE_EDITOR_TEXT_BASE * text_scale` with `spacing` set to
`VOE_EDITOR_SPACING`, and the editor's own pad and gap constants are multiplied by the latter.
Seams, scroll bars, sliders, swatches and panel widths are not scaled.

## Reasoning
On the theme because the theme in force already reaches every widget, pushed copies carry it, and
both built-in themes and file themes go through one derivation in the editor. A context-wide setter
was the alternative; it needs the context's creation in `layout.c`, a file over 800 lines, and a
pushed theme could not differ. The text base is applied at derivation so the slider keeps reading
100 % and a remembered scale shrinks with the base (0306).

## Replaces
nothing
