# 0197 — Slider values are remembered per theme in one settings file
date: 2026-09-19
by: planner

## Decision
The editor remembers a person's contrast and surface separation per theme in
`<settings>/voe3d/theme_scalars`, one line per adjusted theme: `<contrast> <separation> <identity>`, the two
numbers in `%.3f` and the identity the rest of the line, so a file name with a blank in it survives. The
identity is the theme's file name, `near_white` for Near white and `near_black` for Near black (neither can
be a `*.theme` file's name, ADR-0178). A line that does not parse, or whose numbers are outside
`VOE_UI_THEME_SCALAR_MIN..MAX`, is skipped without a report. Lines for themes not listed right now are kept
and written back. The file is written when a drag on either slider ends and on Reset; Reset drops the
theme's line. A theme file is never written. A remembered pair replaces the file's own two scalars at load
and at every live re-read; the file's own values are what Reset puts back.

## Reasoning
ADR-0172 keeps each remembered thing as a sibling file in `<settings>/voe3d/`, and consolidating them into
one settings document is its own decision, so this follows the pattern rather than starting that document.
Numbers first makes the identity the tail of the line and needs no quoting. Writing on the end of a drag,
not every frame of it, keeps a drag from writing a file sixty times a second. Writing the scalars into the
theme file was rejected: the feature says the file is not changed, and Near black and Near white have no
file.

## Replaces
Nothing.
