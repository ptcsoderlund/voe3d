// The Import textures question's open with its title, its one frame of `ui`
// calls with the chosen size lit as levels_ask.c lights its chosen count, and
// the read of its buttons and a press outside it afterwards. See the header
// for what is asked and why 2048 on every opening.
#include "textures_ask.h"

#include "inspector_place.h"
#include "themes.h"

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

// The same plate, padding and gap levels_ask.c's panel uses. Millimetres.
#define ASK_PAD (3.0f * VOE_EDITOR_SPACING)
#define ASK_GAP (2.0f * VOE_EDITOR_SPACING)

// The size every opening chooses (0466 point 4).
#define LARGEST_OPENED 2048u

// The sizes offered, smallest first, each with its button's text.
static const struct {
	uint32_t largest;
	const char *name;
} SIZES[VOE_EDITOR_TEXTURES_ASK_SIZES] = {
	{ 512u, "512" },
	{ 1024u, "1024" },
	{ 2048u, "2048" },
	{ 4096u, "4096" },
};

void voe_editor_textures_ask_open(voe_editor_textures_ask *ask,
				  const char *path)
{
	const char *name;

	VOE_BASE_ASSERT(ask != NULL && path != NULL,
			"asking the textures of no path");

	*ask = (voe_editor_textures_ask){ .open = true,
					  .largest = LARGEST_OPENED,
					  .panel = VOE_UI_NODE_NONE,
					  .import_button = VOE_UI_NODE_NONE,
					  .cancel_button = VOE_UI_NODE_NONE };
	for (uint32_t i = 0; i < VOE_EDITOR_TEXTURES_ASK_SIZES; i++)
		ask->size_buttons[i] = VOE_UI_NODE_NONE;
	snprintf(ask->path, sizeof ask->path, "%s", path);
	name = strrchr(ask->path, '/');
	name = name != NULL ? name + 1 : ask->path;
	snprintf(ask->title, sizeof ask->title, "Import textures of %s", name);
	VOE_BASE_ASSERT(ask->open && ask->title[0] != '\0',
			"a textures question opened with no title");
}

void voe_editor_textures_ask_close(voe_editor_textures_ask *ask)
{
	VOE_BASE_ASSERT(ask != NULL, "closing no textures question");

	ask->open = false;
	VOE_BASE_ASSERT(!ask->open, "a textures question left open");
}

void voe_editor_textures_ask_draw(voe_ui_context *ui,
				  voe_editor_textures_ask *ask, float top)
{
	VOE_BASE_ASSERT(ui != NULL,
			"drawing a textures question into no interface");
	VOE_BASE_ASSERT(ask != NULL && ask->open,
			"drawing a textures question that is not open");

	ask->panel = voe_ui_panel_begin(
		ui, "textures_ask", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.gap = ASK_GAP,
			.pad = { ASK_PAD, ASK_PAD, ASK_PAD, ASK_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_CENTER, 0.0f },
				    .y = { VOE_UI_ACROSS_START, top + ASK_GAP } } });
	voe_ui_label(ui, ask->title);

	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = ASK_GAP });
	voe_ui_label(ui, "Largest size");
	for (uint32_t i = 0; i < VOE_EDITOR_TEXTURES_ASK_SIZES; i++) {
		ask->size_buttons[i] = voe_ui_choice_begin(
			ui, "textures_ask_size", i,
			ask->largest == SIZES[i].largest);
		voe_ui_label(ui, SIZES[i].name);
		voe_ui_end(ui);
	}
	voe_ui_end(ui); // size row

	voe_ui_row_begin(ui, (voe_ui_container){ .gap = ASK_GAP });
	ask->import_button = voe_ui_button_begin(ui, "textures_ask_import", 0);
	voe_ui_label(ui, "Import");
	voe_ui_end(ui); // Import
	ask->cancel_button = voe_ui_button_begin(ui, "textures_ask_cancel", 0);
	voe_ui_label(ui, "Cancel");
	voe_ui_end(ui); // Cancel
	voe_ui_end(ui); // button row
	voe_ui_end(ui); // panel
}

// Whether `node` was laid out this frame and fired.
static bool fired(const voe_ui_context *ui, voe_ui_node node)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a button of no interface");
	return node != VOE_UI_NODE_NONE && voe_ui_button_action(ui, node).fired;
}

voe_editor_textures_ask_answer
voe_editor_textures_ask_read(const voe_ui_context *ui,
			     voe_editor_textures_ask *ask, bool pointer_down,
			     voe_math_float2 at)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(ask != NULL, "reading no textures question");

	// A refused frame hands back VOE_UI_NODE_NONE, skipped as levels_ask.c's
	// are; with no panel laid out, no press is outside it either.
	for (uint32_t i = 0; i < VOE_EDITOR_TEXTURES_ASK_SIZES; i++)
		if (fired(ui, ask->size_buttons[i]))
			ask->largest = SIZES[i].largest;
	if (fired(ui, ask->import_button))
		return VOE_EDITOR_TEXTURES_ASK_IMPORT;
	if (fired(ui, ask->cancel_button))
		return VOE_EDITOR_TEXTURES_ASK_CANCEL;
	if (pointer_down && ask->panel != VOE_UI_NODE_NONE &&
	    !voe_editor_inspector_rect_contains(
		    voe_ui_node_visible(ui, ask->panel), at))
		return VOE_EDITOR_TEXTURES_ASK_CANCEL;
	return VOE_EDITOR_TEXTURES_ASK_NONE;
}
