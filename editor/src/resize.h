// The borders a person drags to size the panels (ADR-0226): every split's
// seam, the side panels', the Assets panel's under the Scene list (ADR-0279)
// and the two scene views' (ADR-0228), and the top bar's lower edge. Each frame
// voe_editor_resize_frame says whether the pointer is the borders' and which
// shape it takes, writes a drag into the tree or the bar, and says when a
// drag or a double-click ended so the caller remembers the sizes through
// panels.h.
//
//     voe_editor_resize resize = { .held = UINT32_MAX };   // at rest
//
// THE EDITOR'S OWN HIT TEST, ASKED BEFORE `ui`'S. A seam is drawn by the
// walk as a fill, not a widget, so no widget would ever answer for it; the
// band is found here from voe_editor_dock_arrange, and while it is the
// pointer's the caller hands `ui`, pick and the gizmo no pointer and no
// press. `reached` is the seam the walk draws lit (ADR-0231, ADR-0232).
//
// A PRESS MUST START OVER A BORDER WITH THE BUTTON UP BEFORE IT. A drag begun
// in a panel or a view that sweeps across a seam is that gesture's, not a
// resize, so hovering needs the button up and holding needs a press edge.
//
// THE DOUBLE-CLICK IS TIMED HERE (ADR-0226): `platform` reports levels, not
// clicks, so two presses on one border within VOE_EDITOR_RESIZE_DOUBLE on
// the caller's clock put that size back to the default tree's: the views to
// an even split (ADR-0228), the bar to fitting its content.
//
// Constraints: a zeroed struct remembers a press on node 0 at the clock's
// origin, which a monotonic clock started with the machine never comes within
// VOE_EDITOR_RESIZE_DOUBLE of. Nothing here writes a file: the sizes are
// remembered through panels.h when a drag ends, never while one goes on.
#pragma once

#include "dock.h"
#include "topbar.h"

#include <platform/input.h>

#include <stdint.h>

// Millimetres the band reaches either side of a seam or the bar's edge.
#define VOE_EDITOR_RESIZE_REACH 0.5f
// Seconds within which a second press on one border is a double-click.
#define VOE_EDITOR_RESIZE_DOUBLE 0.4

// A border is any split's node index, VOE_EDITOR_DOCK_NODES for the top
// bar's lower edge, or UINT32_MAX for none. `held` is the border being
// dragged, `grab` the pointer's offset from its edge at the press along the
// border's axis, `pressed` and `pressed_at` the last press's border and
// time, and `was_down` last frame's button level.
typedef struct {
	uint32_t held;
	float grab;
	uint32_t pressed;
	double pressed_at;
	bool was_down;
} voe_editor_resize;

// The pointer's shape this frame, whether the borders took the pointer, and
// whether a drag or a double-click ended, so the sizes are to be remembered.
// `reached` is the split being dragged, else the one whose band the pointer
// is in with a drag allowed and the button up, else UINT32_MAX (also for the
// bar's edge).
typedef struct {
	voe_platform_cursor cursor;
	bool taken;
	bool ended;
	uint32_t reached;
} voe_editor_resize_result;

// One frame of the borders against `root`'s pointer. `allowed` false while a
// panel shows over the dock: nothing is hovered and no drag starts. `now` is
// seconds on a monotonic clock. Writes a split's edge in `root` through
// voe_editor_dock_split_set or `bar->wanted`, within the bounds a layout keeps.
voe_editor_resize_result voe_editor_resize_frame(voe_editor_resize *resize,
						 voe_editor_dock_root *root,
						 voe_editor_topbar *bar,
						 bool allowed, double now);
