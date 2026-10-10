// The editor's per-frame keyboard commands and the acts on them, lifted out of
// main.c's loop. main.c fills one voe_editor_frame_commands with pointers to
// the parts it owns before the loop, then calls three functions each frame:
// voe_editor_frame_commands_history at the top, before the world step;
// voe_editor_frame_commands_read straight after the fly is read; and
// voe_editor_frame_commands_after_draw inside the window's pass, after the
// interface has drawn.
//
// Moved here from main.c's header, with the code:
//
// WHAT THIS FRAME'S KEYBOARD ASKED FOR IS ONE READ (shortcuts.h): the guards
// main.c holds go in, a flag per shortcut comes back, and every act on one
// is below that read — the three commands through session.h, Delete and Ctrl+D
// through scene.h unless the Assets panel holds the keyboard, F2 through
// assets_panel.h, Ctrl+Z, Ctrl+Shift+Z and Ctrl+Y through undo.h, whose step
// is taken at the top of the next frame, before the structural queue is
// applied. There too, at rest, an edit to the open material is written to its
// file and pushed as one undo step (0399 point 9). Escape's order is this file's (at the picker's close); the panels
// it hides include the Landscape panel, which a different project hides too.
// Escape, Backspace, Enter, Tab and the text read since the last poll are the
// interface's besides (dock.h, keys.h). A flying view keeps every key it reads
// from the interface and the shortcuts, and the pointer from both (view.h).
//
// Constraints: the parts pointed at are main.c's and outlive the loop; none
// of these functions allocates. The fields below the pointers are this file's
// to write; main.c reads `at_rest` and `escape_free` and writes neither.
#pragma once

#include "browser.h"
#include "gizmo.h"
#include "keys.h"
#include "landscape_panel.h"
#include "preferences.h"
#include "project_panel.h"
#include "scene.h"
#include "session.h"
#include "shortcuts.h"
#include "undo.h"
#include "view.h"

#include <platform/input.h>

#include <ui/widgets.h>

#include <stdbool.h>

typedef struct {
	voe_editor_session *session;
	voe_editor_scene *scene;
	voe_editor_browser *browser;
	voe_editor_preferences *preferences;
	voe_editor_project_panel *project_panel;
	voe_editor_landscape_panel *landscape_panel;
	voe_editor_views *views;
	voe_editor_undo *undo;
	voe_editor_models *models;
	voe_editor_gizmo *gizmo;
	voe_ui_context *ui;
	// Working memory for a step's or a settled material's file write,
	// rewound by each.
	voe_base_arena *scratch;
	// This frame's undo and redo edges, acted on at the top of the next
	// frame, and whether the editor was at rest when they were read — the
	// same rest a step is recorded at (undo.h).
	bool step_back;
	bool step_forward;
	bool at_rest;
	// What this frame's keyboard asked for (shortcuts.h).
	voe_editor_shortcuts shortcuts;
	// This frame's Escape edge when neither typing nor the picker took it:
	// the browser's Cancel and Preferences' Close. This file's own, because
	// closing the picker spends the edge.
	bool escape_free;
} voe_editor_frame_commands;

// The open material's edit settled at rest, then a different project, a prefab
// opened or Back, or last frame's Ctrl+Z or Ctrl+Y, acted on at the top of the
// frame, before world_step.h.
void voe_editor_frame_commands_history(voe_editor_frame_commands *commands);

// This frame's shortcuts read against the guards and acted on, Escape's order
// run, and the keyboard `ui` is handed this frame returned.
voe_ui_keyboard
voe_editor_frame_commands_read(voe_editor_frame_commands *commands,
			       const voe_editor_keys_frame *keyboard,
			       const voe_platform_text *text, bool left,
			       bool flying);

// Delete, Ctrl+D and R, the scene's full notice, and an edit marked unsaved and
// for undo, inside the window's pass once the interface has drawn.
void voe_editor_frame_commands_after_draw(voe_editor_frame_commands *commands);
