# 0177 — To see what was drawn, render to a PNG
date: 2026-09-18
by: tech-lead

## Decision
A coder who needs to check what the engine drew renders it to a PNG. They never screenshot the programmer's
display. `./build/debug/editor/voe_editor --capture <path>.png [--size <W>x<H>]` draws one frame with no window,
writes the PNG and exits; the coder then reads the PNG. A program of the coder's own does the same through
`voe_app_new_headless` and `voe_app_capture_png` (ADR-0157). The planner names this decision on every card that
changes what is drawn: layout, widgets, themes, fonts, anything a person would see. It does not name it on other
cards.

## Reasoning
This text was a `## Seeing what was drawn` section in `CLAUDE.md`. The workflow allows three headings there, so
`checks.sh --all` failed and blocked 006's card 10.
- **Allow the heading in the workflow (`agentic_rules`)**: rejected. It would loosen the check for every product
  to fit one product's need.
- **Put it under `## Checks`**: rejected. `checks.sh` runs each entry there as a command.
- **Fold it into 0168, which every card names**: rejected. Most cards draw nothing. 0168 keeps rules out of
  `CLAUDE.md` so that an agent loads only what its card needs, and the same reasoning keeps this out of 0168. A
  separate decision reaches only the coders who need it.

## Replaces
The `## Seeing what was drawn` section of `CLAUDE.md`, now removed.
