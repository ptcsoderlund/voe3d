// The Delete question's open with its one walk, its one frame of `ui` calls
// and the read of its buttons and of a press outside it afterwards. See the
// header for why the walk runs once.
#include "assets_ask.h"

#include "inspector_place.h"
#include "themes.h"

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

// The same plate, padding and gap errors.c's panel uses. Millimetres.
#define ASK_PAD (3.0f * VOE_EDITOR_SPACING)
#define ASK_GAP (2.0f * VOE_EDITOR_SPACING)

// The used-by line: up to the kept names, then "and N more" for the rest.
static void used_by_write(voe_editor_assets_ask *ask)
{
	size_t at;
	uint32_t kept;

	VOE_BASE_ASSERT(ask != NULL, "writing the users of no question");
	VOE_BASE_ASSERT(ask->user_count > 0, "a used-by line with no users");
	kept = ask->user_count < VOE_EDITOR_ASSETS_USERS_KEPT ?
		       ask->user_count :
		       VOE_EDITOR_ASSETS_USERS_KEPT;
	snprintf(ask->used_by, sizeof ask->used_by, "Used by %s", ask->users[0]);
	for (uint32_t i = 1; i < kept; i++) {
		at = strlen(ask->used_by);
		snprintf(ask->used_by + at, sizeof ask->used_by - at, ", %s",
			 ask->users[i]);
	}
	at = strlen(ask->used_by);
	if (ask->user_count > kept)
		snprintf(ask->used_by + at, sizeof ask->used_by - at,
			 " and %u more", (unsigned)(ask->user_count - kept));
	VOE_BASE_ASSERT(strlen(ask->used_by) <= VOE_EDITOR_ASSETS_ASK_LINE,
			"a used-by line past its cut");
}

void voe_editor_assets_ask_open(voe_editor_assets_ask *ask,
				const char *project_folder, const char *path,
				voe_base_arena *arena)
{
	voe_editor_assets_used used = { 0 };
	char walked[VOE_EDITOR_ASSETS_PATH + sizeof "Assets/"];
	const char *name;

	VOE_BASE_ASSERT(ask != NULL && path != NULL && path[0] != '\0',
			"asking about no path");
	VOE_BASE_ASSERT(arena != NULL, "asking with no arena for the users");

	*ask = (voe_editor_assets_ask){ .open = true,
					.panel = VOE_UI_NODE_NONE,
					.delete_button = VOE_UI_NODE_NONE,
					.cancel_button = VOE_UI_NODE_NONE };
	snprintf(ask->path, sizeof ask->path, "%s", path);
	name = strrchr(ask->path, '/');
	name = name != NULL ? name + 1 : ask->path;
	snprintf(ask->question, sizeof ask->question, "Delete %s?", name);
	if (project_folder != NULL) {
		snprintf(walked, sizeof walked, "Assets/%s", ask->path);
		voe_editor_assets_users(project_folder, walked, arena, &used);
	}
	ask->user_count = used.count;
	for (uint32_t i = 0; i < used.count && i < VOE_EDITOR_ASSETS_USERS_KEPT;
	     i++)
		snprintf(ask->users[i], sizeof ask->users[i], "%s",
			 used.names[i]);
	if (ask->user_count > 0)
		used_by_write(ask);
	VOE_BASE_ASSERT(ask->open, "a question opened closed");
}

void voe_editor_assets_ask_close(voe_editor_assets_ask *ask)
{
	VOE_BASE_ASSERT(ask != NULL, "closing no question");

	ask->open = false;
	VOE_BASE_ASSERT(!ask->open, "a question left open");
}

void voe_editor_assets_ask_draw(voe_ui_context *ui, voe_editor_assets_ask *ask,
				float top)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing a question into no interface");
	VOE_BASE_ASSERT(ask != NULL && ask->open,
			"drawing a question that is not open");

	ask->panel = voe_ui_panel_begin(
		ui, "assets_ask", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.gap = ASK_GAP,
			.pad = { ASK_PAD, ASK_PAD, ASK_PAD, ASK_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_CENTER, 0.0f },
				    .y = { VOE_UI_ACROSS_START, top + ASK_GAP } } });
	voe_ui_label(ui, ask->question);
	if (ask->user_count > 0)
		voe_ui_label(ui, ask->used_by);
	voe_ui_row_begin(ui, (voe_ui_container){ .gap = ASK_GAP });
	ask->delete_button = voe_ui_button_begin(ui, "assets_ask_delete", 0);
	voe_ui_label(ui, "Delete");
	voe_ui_end(ui); // Delete
	ask->cancel_button = voe_ui_button_begin(ui, "assets_ask_cancel", 0);
	voe_ui_label(ui, "Cancel");
	voe_ui_end(ui); // Cancel
	voe_ui_end(ui); // button row
	voe_ui_end(ui); // panel
}

voe_editor_assets_ask_answer
voe_editor_assets_ask_read(const voe_ui_context *ui,
			   const voe_editor_assets_ask *ask, bool pointer_down,
			   voe_math_float2 at)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(ask != NULL, "reading no question");

	// A refused frame hands back VOE_UI_NODE_NONE, skipped as errors.c's
	// are; with no panel laid out, no press is outside it either.
	if (ask->delete_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, ask->delete_button).fired)
		return VOE_EDITOR_ASSETS_ASK_DELETE;
	if (ask->cancel_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, ask->cancel_button).fired)
		return VOE_EDITOR_ASSETS_ASK_CANCEL;
	if (pointer_down && ask->panel != VOE_UI_NODE_NONE &&
	    !voe_editor_inspector_rect_contains(
		    voe_ui_node_visible(ui, ask->panel), at))
		return VOE_EDITOR_ASSETS_ASK_CANCEL;
	return VOE_EDITOR_ASSETS_ASK_NONE;
}
