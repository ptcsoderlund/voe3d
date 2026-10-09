// The Assets panel's share of interface.c's one read: what its rows, keyboard,
// naming field, right-button menu and Delete question asked for this frame,
// carried out. Called by voe_editor_interface_draw only, between
// voe_ui_frame_end and the arena's rewind, after the Scene list's drop and the
// selection's reveal and before the views' rectangles are read.
//
// APART FROM interface.c BECAUSE IT IS ONE PANEL'S COMMANDS AND NOT THE FRAME.
// interface.c opens the frame, walks the dock and draws over it; what a fired
// Assets row means is said here, in the order its reads must keep.
#pragma once

#include "interface.h"

// Carries out the Assets panel's frame on `root`: its clicks read (a fired
// Import shows `browser`), its naming request through assets_manage.h, a made
// or fired landscape opened in `landscape_panel` in place of `preferences` and
// `project_panel`, the right-button menu's row when `menuing`, the Delete
// question's answer when `asking` and a new question opened, then a fired
// prefab opened (session.h). `over_assets` is whether the pointer is over the
// Assets leaf. Scratch comes out of `arena`, which the caller rewinds.
void voe_editor_interface_assets_read(
	voe_ui_context *ui, voe_base_arena *arena,
	const voe_editor_dock_root *root, voe_editor_scene *scene,
	voe_editor_undo *undo, voe_editor_models *models,
	voe_editor_session *session, voe_editor_browser *browser,
	voe_editor_preferences *preferences,
	voe_editor_project_panel *project_panel,
	voe_editor_landscape_panel *landscape_panel, bool over_assets,
	bool menuing, bool asking);
