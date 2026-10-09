// The rest of interface.c's one read that is not the Assets panel's: the
// Inspector's, the colour picker's and the Scene list's, and the commands the
// dock's ×, the Panels list, the top bar, the browser, Preferences and the
// Project, Landscape, Errors and frame breakdown panels fired this frame,
// carried out. Called by voe_editor_interface_draw only, between
// voe_ui_frame_end and the arena's rewind, at the two places in its order it
// names.
//
// APART FROM interface.c BECAUSE THESE ARE COMMANDS AND NOT THE FRAME.
// interface.c opens the frame, walks the dock and draws over it; what a fired
// button there means is said here.
#pragma once

#include "interface.h"

#include <ui/colour.h>

// Which panels the frame was built with, captured before anything was drawn so
// a click is read against what was laid out, not what this frame's commands
// have since shown or hidden (interface.h on `browsing`).
typedef struct {
	bool browsing;
	bool erroring;
	bool preferring;
	bool projecting;
	bool landscaping;
	bool framing;
	// The Panels list was open.
	bool menuing;
} voe_editor_interface_shown;

// The first read on `root`: the Inspector's edits, the picker's `picker` node
// (VOE_UI_NODE_NONE when it was not drawn) on `picked` as it was when drawn,
// the Inspector's buttons and Sculpt section, the Scene list's clicks and drop
// with its prefab made in `arena`, then the reveal. A full scene and a refused
// make are said in `session`'s notice. True when the pointer is over the
// Assets leaf below `bar`, which the Assets panel's read is handed.
bool voe_editor_interface_scene_read(voe_ui_context *ui, voe_base_arena *arena,
				     const voe_editor_dock_root *root,
				     voe_editor_scene *scene,
				     voe_editor_session *session,
				     const voe_editor_topbar *bar,
				     voe_ui_node picker,
				     const voe_editor_picking *picked);

// The dock's × in `closes` and the Panels list toggle a panel on `root`, a
// failed remember said in the notice; the bar is measured for the next frame;
// then the browser's clicks if it was `shown`, else the bar's, then each panel
// over the dock that was shown. `escape` is the browser's Cancel. Scratch comes
// out of `arena`, which the caller rewinds.
void voe_editor_interface_commands_read(
	voe_ui_context *ui, voe_base_arena *arena, voe_editor_dock_root *root,
	const voe_editor_dock_closes *closes, voe_editor_scene *scene,
	voe_editor_models *models, voe_editor_session *session,
	voe_editor_topbar *bar, voe_editor_browser *browser,
	voe_editor_preferences *preferences,
	voe_editor_project_panel *project_panel,
	voe_editor_landscape_panel *landscape_panel, voe_editor_themes *themes,
	voe_editor_frame_breakdown *breakdown, voe_editor_interface_shown shown,
	bool escape);
