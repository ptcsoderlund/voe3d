// The drag ghost's one frame of `ui` calls: the dim pushed when refused, the
// anchored panel, the name and the refusal's line (drag_ghost.h).
#include "drag_ghost.h"

#include <base/assert.h>

// How far right of and below the pointer the ghost sits, in millimetres.
#define GHOST_OFFSET 3.0f

// The ghost's padding round its labels, in millimetres.
#define GHOST_PAD 0.5f

void voe_editor_drag_ghost_draw(voe_ui_context *ui, const voe_ui_theme *dim,
				const char *name, bool refused,
				voe_math_float2 at)
{
	voe_ui_node ghost;

	VOE_BASE_ASSERT(ui != NULL, "drawing a ghost into no interface");
	VOE_BASE_ASSERT(name != NULL, "drawing a ghost with no name");
	VOE_BASE_ASSERT(!refused || dim != NULL,
			"drawing a refused ghost with no dim theme");
	if (refused)
		voe_ui_theme_push(ui, dim);
	// Not blocking the pointer, so what is under it still answers it.
	ghost = voe_ui_panel_begin(
		ui, "drag_ghost", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.pad = { GHOST_PAD, GHOST_PAD, GHOST_PAD, GHOST_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_START,
					   at.x + GHOST_OFFSET },
				    .y = { VOE_UI_ACROSS_START,
					   at.y + GHOST_OFFSET } } });
	voe_ui_label(ui, name);
	if (refused)
		voe_ui_label(ui, "Can't drop here");
	voe_ui_end(ui);
	if (refused)
		voe_ui_theme_pop(ui);
	VOE_BASE_ASSERT(ghost != VOE_UI_NODE_NONE, "a ghost with no node");
}
