# 0178 — Two built-in themes, Near black and Near white, the light one remembered as `near_white`
date: 2026-09-18
by: planner

## Decision
The editor has two compiled-in themes: entry 0 `Near black` (`voe_ui_theme_default_inputs()`, Oxanium) and
entry 1 `Near white` (the same inputs with `mode = VOE_UI_THEME_MODE_LIGHT`, Oxanium). Neither has a file, and
neither is re-read by the live check. Theme files follow them in the list. `<settings>/voe3d/theme` holds a
file theme's file name as before. It holds `near_white` for Near white, and it is empty or absent for Near
black. Any other line that names no listed file falls back to Near black with the notice it already raises.

## Reasoning
Spec 006 now asks for two built-ins so that switching can be tried without writing a file. ADR-0172 remembers a
theme by its file name, and a built-in has no file. Only `*.theme` files are listed, so a line without that
suffix can never be a file's name, and `near_white` cannot collide with one. An empty line still means the dark
default, so a choice remembered before this change keeps working.
- **Store the display name**: rejected. ADR-0172 keeps display names out of identity.
- **Ship Near white as a file written into the themes folder**: rejected. The file would be the person's to
  break or delete, and a built-in should always be there.

## Replaces
ADR-0172's "the built-in theme" (singular) and "empty or absent meaning the built-in": the empty line now means
Near black, and `near_white` means Near white.
