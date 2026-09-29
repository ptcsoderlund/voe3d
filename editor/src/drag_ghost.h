// The ghost every editor drag shows beside the pointer (ADR-0285, 0286 point
// 1): a raised panel of the dragged thing's name, and, when a release there
// would drop nothing, the same panel dimmed with a second line, "Can't drop
// here". A drag's own file works out whether it is refused and calls this
// once a frame while the drag is under way:
//
//     voe_editor_drag_ghost_draw(ui, &scene->list_dim, name, refused, at);
//
// Constraints: called in the root surface, `at` in its millimetres, so the
// anchor is the surface's. `name` and `dim` must outlive the frame: `ui` keeps
// the pointers until its records are built.
#pragma once

#include <math/float2.h>
#include <ui/theme.h>
#include <ui/widgets.h>

#include <stdbool.h>

// An anchored raised panel of `name` a few millimetres right of and below
// `at`, taking no pointer; when `refused`, drawn under `dim` with "Can't drop
// here" below the name.
void voe_editor_drag_ghost_draw(voe_ui_context *ui, const voe_ui_theme *dim,
				const char *name, bool refused,
				voe_math_float2 at);
