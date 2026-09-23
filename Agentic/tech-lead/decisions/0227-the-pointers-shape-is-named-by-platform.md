# 0227 — The pointer's shape is asked of `platform` by name
date: 2026-09-23
by: planner

## Decision
For 021. `platform` gains `voe_platform_input_cursor(window, shape)` with three shapes: the arrow,
left-right and up-down. It is plain state in `src/input.h`, applied by each backend when it changes and
when the pointer enters. Wayland uses the optional `cursor-shape-v1` protocol, vendored with the
`tablet-v2` XML its generated code names; without it the shape does not change and nothing is reported.
Windows answers `WM_SETCURSOR` in the client area with the system cursor for the shape. There is still no
cursor image in the engine.

## Reasoning
A named shape needs no image: the compositor or the system draws its own, in the person's theme and size.
`libwayland-cursor` would load a theme and draw it ourselves onto a surface, which is more code and a
library. Optional like every other protocol here, so an older compositor still runs the editor.

## Replaces
Nothing. `include/platform/input.h`'s note that no card owns a cursor yet is narrowed: hiding one for the
lock is still unowned.
