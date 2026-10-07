# 0380 — The editor names assets, to read and write landscapes
date: 2026-10-07
by: planner

## Decision
`assets` joins the editor's row of allowed dependencies in `cmake/voe.cmake` and the editor's
`DEPENDS`. The editor already reads, makes and resizes `.landscape` files through
`assets/landscape.h` (082: Create, the Landscape panel, the game tree's cook); the edge makes the
include it has honest. The editor still decodes no glTF, image or sound itself; those stay with
`3d`, `audio` and `theme`.

## Reasoning
The editor is a leaf (0121), so a new edge into it makes no cycle, and `assets` is already linked
to it through `authoring` and `3d`. The other way, wrapping the landscape reader and writer in
`authoring`, adds names that only forward calls. `check.cmake`'s includes step refuses the include
without the edge, which is how 082's suite failed.

## Replaces
Nothing. Extends 0022 and 0121; amends the "assets stays absent" note on the editor's row.
