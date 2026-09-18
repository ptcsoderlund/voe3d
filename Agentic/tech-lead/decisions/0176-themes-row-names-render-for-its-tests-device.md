# 0176 — `theme`'s row names `render`, for its test's device
date: 2026-09-18
by: planner

## Decision
`theme`'s row in `cmake/voe.cmake` is `ui text render assets math base`: record 0170's row plus `render`. The one
use is `theme/tests/theme.c`, which makes a headless device (`voe_render_device_new_headless`) and a font on it,
because a label and a button measure a string and a NULL font asserts. `theme/src/` and `theme/include/` name no
`render/` header; the folder's `.md` says so.

## Reasoning
Criterion 10 of 006 asks for a theme read from a file deciding a panel's, a label's and a button's colours with no
editor involved, and `check.cmake` step 5 refuses any `#include <render/...>` in a folder whose `DEPENDS` lacks
`render`, tests included. Record 0170 wrote its row without that test in view. `render` is already below `ui` and
`text`, which `theme` names, so the edge adds no cycle and links nothing new.
- The check with only a panel, which needs no font: leaves out the label and button the criterion names.
- The check in `ui`: `ui` may not name `assets` (ADR-0093), so it cannot read a theme file.
- The check in `editor`: the criterion rules it out.

## Replaces
nothing; it widens 0170's row by one folder.
