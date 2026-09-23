# 0226 — A side panel is held at a length in millimetres, kept in `editor_settings`
date: 2026-09-23
by: planner

## Decision
For 021. A dock split may **hold** its first or second child at a length in the surface's millimetres
instead of dividing by `fraction`; the default tree holds the Scene list and the Inspector at 48 mm each (a
fifth of a 16:9 surface), and the views take the rest. A held length is clamped when laid out: no less than
`VOE_EDITOR_DOCK_PANEL_MIN` (30 mm), and never so much that the other side has less than what it needs — a
scene view `VOE_EDITOR_DOCK_VIEW_ROOM` (40 mm) on the split's axis, a held neighbour its own length. Where
the two cannot both hold, the view's room wins.

The top bar keeps 0225's measured height as its **least**; the person's height is a second number, nought
meaning "fit the content", and the bar is the larger of the two, never so tall that the dock has less than
`VOE_EDITOR_DOCK_VIEW_ROOM`.

A border is the seam's gap widened by half a millimetre either side, and the bar's lower edge the same band.
A drag starts on a press over a border with the button up before it; two presses on the same border within
0.4 s are a double-click and put that one size back to the default tree's (the bar's to nought).

The sizes are kept (0220) in `<settings>/voe3d/editor_settings`, one `<key> <number>` line each —
`scene_wide`, `inspector_wide`, `topbar_high`, `%.3f` millimetres — written when a drag or a double-click
ends. A missing file, a line that does not parse or a number out of range leaves that size at its default
without a report; lines with other keys are kept and written back, so later settings join this file.

## Reasoning
Millimetres and not a fraction, so a panel keeps its width when the window changes and a minimum is a
length a person can use. A named-key file because 0220 calls it the editor's settings file and later
settings belong in it; the older sibling files (0197, last project, theme) are not moved here. A
double-click is timed in the editor on the clock it already has; `platform` reports levels, not clicks.

## Replaces
Amends 0225: the measured height is the bar's least, not its height.
