# 0402 — A slider that cannot move is drawn without its number box
date: 2026-10-11
by: planner

## Decision
For 085 bug 02 (0401's greyed-out Roughness and Metal). `ui` gains `voe_ui_slider_shown` in
`slider.h`: the same track and thumb `voe_ui_slider` draws, at the same size and the thumb at the
value's place, but with no number box round them, so nothing about it answers the pointer or the
keyboard: no drag, no click into typing, no Tab stop, no action to read. It takes the theme in force;
the caller greys it by pushing a theme whose text is `text_disabled` and whose fills do not light. The
editor's material section draws Roughness and Metal this way, under its own dimmed theme, while the
ORM slot is filled.

## Reasoning
- A slider drawn as usual and left unread still inverts under a drag and opens for typing on a click,
  so it seems to do something it does not; 0401 rules that out.
- A disabled flag on every widget is a wider change to `ui` than one call needs; one call beside the
  slider, built from the same parts, is the least.
- Drawing a track and a thumb by hand in the editor repeats `slider.c` and drifts from it.

## Replaces
Nothing.
