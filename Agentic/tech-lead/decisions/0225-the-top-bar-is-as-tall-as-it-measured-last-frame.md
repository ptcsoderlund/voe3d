# 0225 — The top bar is as tall as its content measured the frame before
date: 2026-09-23
by: planner

## Decision
`VOE_EDITOR_TOPBAR_HIGH` stops being the bar's height and becomes the height of the first frame, before
anything has been measured. Each frame the bar is laid out at the height its panel's content measured
(`voe_ui_node_measured`) in the previous frame, and the dock tree gets the rest of the surface. The row
inside the panel takes its natural height, so the measure is what the labels and buttons need and not the
height the bar was given.

## Reasoning
0219 has elements fit their content, and at twice the text size a 10 mm bar cuts its buttons. `dock.c`
divides a known length into fixed millimetres, so the bar's height has to be a number before the frame is
built, and an immediate-mode layout only knows what content needed after the frame ends. Last frame's
measure is that number with no new `ui` surface; while a slider is dragged it trails by one frame, which
is not visible. Rejected: working the height out from the font's line height and `ui`'s button padding,
which copies `ui`'s private padding into the editor; letting the dock row grow, which undoes dock.c's
reason for fixed sizes.

## Replaces
Nothing.
