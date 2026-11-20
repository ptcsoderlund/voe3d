// The disk question's open with its line, its one frame of `ui` calls, and the
// read of its two buttons afterwards. See the header for why nothing else
// answers it.
#include "disk_ask.h"

#include "themes.h"

#include <base/assert.h>

#include <stdio.h>

// The same plate, padding and gap textures_ask.c's panel uses. Millimetres.
#define ASK_PAD (3.0f * VOE_EDITOR_SPACING)
#define ASK_GAP (2.0f * VOE_EDITOR_SPACING)

void voe_editor_disk_ask_open(voe_editor_disk_ask *ask, const char *path)
{
	VOE_BASE_ASSERT(ask != NULL && path != NULL,
			"asking about a change to no path");

	*ask = (voe_editor_disk_ask){ .open = true,
				      .panel = VOE_UI_NODE_NONE,
				      .load_button = VOE_UI_NODE_NONE,
				      .keep_button = VOE_UI_NODE_NONE };
	snprintf(ask->line, sizeof ask->line, "%.*s%s",
		 VOE_EDITOR_DISK_ASK_PATH - 1, path, VOE_EDITOR_DISK_ASK_SAID);
	VOE_BASE_ASSERT(ask->open && ask->line[0] != '\0',
			"a disk question opened with no line");
}

void voe_editor_disk_ask_close(voe_editor_disk_ask *ask)
{
	VOE_BASE_ASSERT(ask != NULL, "closing no disk question");

	ask->open = false;
	VOE_BASE_ASSERT(!ask->open, "a disk question left open");
}

void voe_editor_disk_ask_draw(voe_ui_context *ui, voe_editor_disk_ask *ask,
			      float top)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing a disk question into no interface");
	VOE_BASE_ASSERT(ask != NULL && ask->open,
			"drawing a disk question that is not open");

	ask->panel = voe_ui_panel_begin(
		ui, "disk_ask", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.gap = ASK_GAP,
			.pad = { ASK_PAD, ASK_PAD, ASK_PAD, ASK_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_CENTER, 0.0f },
				    .y = { VOE_UI_ACROSS_START, top + ASK_GAP } } });
	voe_ui_label(ui, ask->line);

	voe_ui_row_begin(ui, (voe_ui_container){ .gap = ASK_GAP });
	ask->load_button = voe_ui_button_begin(ui, "disk_ask_load", 0);
	voe_ui_label(ui, "Load from disk");
	voe_ui_end(ui); // Load from disk
	ask->keep_button = voe_ui_button_begin(ui, "disk_ask_keep", 0);
	voe_ui_label(ui, "Keep mine");
	voe_ui_end(ui); // Keep mine
	voe_ui_end(ui); // button row
	voe_ui_end(ui); // panel
}

// Whether `node` was laid out this frame and fired.
static bool fired(const voe_ui_context *ui, voe_ui_node node)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a button of no interface");
	return node != VOE_UI_NODE_NONE && voe_ui_button_action(ui, node).fired;
}

voe_editor_disk_ask_answer voe_editor_disk_ask_read(const voe_ui_context *ui,
						    const voe_editor_disk_ask *ask,
						    bool pointer_down,
						    voe_math_float2 at)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(ask != NULL, "reading no disk question");

	// A press outside answers nothing (see the header); taken only for
	// the shape every question's read has.
	(void)pointer_down;
	(void)at;
	// A refused frame hands back VOE_UI_NODE_NONE, skipped as
	// textures_ask.c's are.
	if (fired(ui, ask->load_button))
		return VOE_EDITOR_DISK_ASK_LOAD;
	if (fired(ui, ask->keep_button))
		return VOE_EDITOR_DISK_ASK_KEEP;
	return VOE_EDITOR_DISK_ASK_NONE;
}
