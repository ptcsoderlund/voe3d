// The question the Delete key asks about the Assets panel's selected row
// before it goes to the desktop's trash (ADR-0377 point 3): a line asking to
// delete the row's name, a line naming what uses it, and Delete and Cancel, as
// an anchored panel over the dock below the bar, drawn and read the way
// errors.h's panel is.
//
//     voe_editor_assets_ask_open(&ask, project_folder, "Rocks/a.glb", arena);
//     voe_editor_assets_ask_draw(ui, &ask, top);
//     ... voe_ui_frame_end ...
//     answer = voe_editor_assets_ask_read(ui, &ask, pointer_down, at);
//
// THE USERS ARE WALKED ONCE, WHEN THE QUESTION OPENS (assets_walk.h), never per
// frame: the walk reads every scene and prefab in the project. Up to
// VOE_EDITOR_ASSETS_USERS_KEPT are named, then "and N more". Nothing here
// trashes or closes: the caller carries out the answer (interface.c).
//
// Constraints. Both lines are cut at VOE_EDITOR_ASSETS_ASK_LINE bytes, which is
// what interface.h's element budget counts; a cut may split a UTF-8 character
// at the very end. A user's name is cut at VOE_EDITOR_ASSETS_PATH_ROOM. An
// untitled project walks nothing and names no user.
#pragma once

#include "assets_panel.h"
#include "assets_walk.h"

#include <base/arena.h>

#include <math/float2.h>

#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

// The most bytes of either line kept, not counting its NUL.
#define VOE_EDITOR_ASSETS_ASK_LINE 320

// Zeroed is a question never asked.
typedef struct {
	bool open;
	// Relative to `Assets/`, `/` between, as assets_manage.h takes it.
	char path[VOE_EDITOR_ASSETS_PATH];
	// The first users' project-relative names and how many in all.
	char users[VOE_EDITOR_ASSETS_USERS_KEPT][VOE_EDITOR_ASSETS_PATH_ROOM];
	uint32_t user_count;
	// Kept here because a label's text is drawn after the call that made it.
	char question[VOE_EDITOR_ASSETS_ASK_LINE + 1];
	char used_by[VOE_EDITOR_ASSETS_ASK_LINE + 1];
	voe_ui_node panel;
	voe_ui_node delete_button;
	voe_ui_node cancel_button;
} voe_editor_assets_ask;

typedef enum {
	VOE_EDITOR_ASSETS_ASK_NONE = 0,
	VOE_EDITOR_ASSETS_ASK_DELETE,
	VOE_EDITOR_ASSETS_ASK_CANCEL,
} voe_editor_assets_ask_answer;

// Opens the question about `path`, its users walked under `project_folder`
// (NULL for untitled: none) with names pushed into `arena`, which the caller
// rewinds.
void voe_editor_assets_ask_open(voe_editor_assets_ask *ask,
				const char *project_folder, const char *path,
				voe_base_arena *arena);
void voe_editor_assets_ask_close(voe_editor_assets_ask *ask);

// Draws the question centred across the surface, `top` below its top, over the
// dock, and records its nodes. Asserts when it is not open.
void voe_editor_assets_ask_draw(voe_ui_context *ui, voe_editor_assets_ask *ask,
				float top);

// After voe_ui_frame_end: Delete or Cancel fired, or a `pointer_down` at `at`
// outside the panel, which is a Cancel; NONE otherwise.
voe_editor_assets_ask_answer
voe_editor_assets_ask_read(const voe_ui_context *ui,
			   const voe_editor_assets_ask *ask, bool pointer_down,
			   voe_math_float2 at);
