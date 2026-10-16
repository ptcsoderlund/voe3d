// The levels question's open with its one quiet read of the `.parts`, its one
// frame of `ui` calls with the chosen count, strength and swap lit as
// inspector_sculpt.c lights its chosen kind, the read of its buttons and of a
// press outside it afterwards, and Make's save into the `.parts`. See the
// header for what is shown and what a save refuses.
#include "levels_ask.h"

#include "inspector_place.h"
#include "themes.h"

#include <base/assert.h>
#include <platform/file.h>
#include <platform/path.h>

#include <stdio.h>
#include <string.h>

// The same plate, padding and gap assets_ask.c's panel uses. Millimetres.
#define ASK_PAD (3.0f * VOE_EDITOR_SPACING)
#define ASK_GAP (2.0f * VOE_EDITOR_SPACING)

#define GLB_ENDING ".glb"
#define PARTS_ENDING ".parts"

// A `.parts` path's room: the `.glb`'s, its ending two bytes longer.
#define PARTS_PATH (VOE_3D_MODEL_PATH + sizeof PARTS_ENDING - sizeof GLB_ENDING)

static const char *const COUNT_NAMES[VOE_ASSETS_LEVELS] = {
	"1", "2", "3", "4", "5", "6"
};

static const char *const STRENGTH_NAMES[VOE_EDITOR_LEVELS_ASK_STRENGTHS] = {
	"Gentle", "Medium", "Strong"
};

static const char *const SWAP_NAMES[VOE_EDITOR_LEVELS_ASK_SWAPS] = {
	"Near", "Normal", "Far", "Farther"
};

// `c` in lower case when it is an ASCII capital.
static char lower(char c)
{
	return c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : c;
}

// `glb`'s `.glb` ending, any case, made `.parts` into `out` of PARTS_PATH.
static void parts_path_of(const char *glb, char *out)
{
	const size_t length = strlen(glb);
	const size_t tail = sizeof GLB_ENDING - 1;
	size_t k = 0;

	VOE_BASE_ASSERT(length > tail && length < VOE_3D_MODEL_PATH,
			"a .glb path with no ending or past its room");
	while (k < tail && lower(glb[length - tail + k]) == GLB_ENDING[k])
		k++;
	VOE_BASE_ASSERT(k == tail, "asking the levels of a file that is not a .glb");
	snprintf(out, PARTS_PATH, "%.*s" PARTS_ENDING, (int)(length - tail), glb);
}

// The `.parts` at `full` read into `parts`. False when it will not read.
static bool parts_loaded(const char *full, voe_base_arena *scratch,
			 voe_assets_parts *parts)
{
	const uint8_t *bytes;
	size_t count = 0;

	VOE_BASE_ASSERT(full != NULL && parts != NULL, "loading no parts");
	bytes = voe_platform_file_read(full, scratch, &count, NULL);
	return bytes != NULL &&
	       voe_assets_parts_read((const char *)bytes, count, parts, NULL);
}

void voe_editor_levels_ask_open(voe_editor_levels_ask *ask,
				const char *project_folder, const char *path,
				voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark;
	voe_assets_parts there = { 0 };
	char parts_path[PARTS_PATH];
	const char *full;
	const char *name;

	VOE_BASE_ASSERT(ask != NULL && path != NULL && project_folder != NULL,
			"asking the levels of no path");
	VOE_BASE_ASSERT(scratch != NULL, "asking the levels with no scratch");

	*ask = (voe_editor_levels_ask){ .open = true,
					.levels = VOE_ASSETS_LEVELS_DEFAULT,
					.panel = VOE_UI_NODE_NONE,
					.make_button = VOE_UI_NODE_NONE,
					.cancel_button = VOE_UI_NODE_NONE };
	for (uint32_t i = 0; i < VOE_ASSETS_LEVELS; i++)
		ask->count_buttons[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_LEVELS_ASK_STRENGTHS; i++)
		ask->strength_buttons[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_LEVELS_ASK_SWAPS; i++)
		ask->swap_buttons[i] = VOE_UI_NODE_NONE;
	snprintf(ask->path, sizeof ask->path, "%s", path);
	name = strrchr(ask->path, '/');
	name = name != NULL ? name + 1 : ask->path;
	snprintf(ask->title, sizeof ask->title, "LODs of %s", name);

	parts_path_of(ask->path, parts_path);
	mark = voe_base_arena_mark(scratch);
	full = voe_platform_path_join(scratch, project_folder, parts_path);
	if (voe_platform_file_exists(full) && parts_loaded(full, scratch, &there))
		ask->levels = there.levels;
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(ask->levels.count >= 1 &&
				ask->levels.count <= VOE_ASSETS_LEVELS,
			"a levels question opened with a count out of range");
}

void voe_editor_levels_ask_close(voe_editor_levels_ask *ask)
{
	VOE_BASE_ASSERT(ask != NULL, "closing no levels question");

	ask->open = false;
	VOE_BASE_ASSERT(!ask->open, "a levels question left open");
}

void voe_editor_levels_ask_draw(voe_ui_context *ui, voe_editor_levels_ask *ask,
				float top)
{
	const voe_ui_container row = { .across = VOE_UI_ACROSS_CENTER,
				       .gap = ASK_GAP };

	VOE_BASE_ASSERT(ui != NULL, "drawing a levels question into no interface");
	VOE_BASE_ASSERT(ask != NULL && ask->open,
			"drawing a levels question that is not open");

	ask->panel = voe_ui_panel_begin(
		ui, "levels_ask", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.gap = ASK_GAP,
			.pad = { ASK_PAD, ASK_PAD, ASK_PAD, ASK_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_CENTER, 0.0f },
				    .y = { VOE_UI_ACROSS_START, top + ASK_GAP } } });
	voe_ui_label(ui, ask->title);

	voe_ui_row_begin(ui, row);
	voe_ui_label(ui, "LODs");
	for (uint32_t i = 0; i < VOE_ASSETS_LEVELS; i++) {
		ask->count_buttons[i] = voe_ui_choice_begin(
			ui, "levels_ask_count", i, ask->levels.count == i + 1);
		voe_ui_label(ui, COUNT_NAMES[i]);
		voe_ui_end(ui);
	}
	voe_ui_end(ui); // LODs row

	voe_ui_row_begin(ui, row);
	voe_ui_label(ui, "Strength");
	for (uint32_t i = 0; i < VOE_EDITOR_LEVELS_ASK_STRENGTHS; i++) {
		ask->strength_buttons[i] = voe_ui_choice_begin(
			ui, "levels_ask_strength", i,
			(uint32_t)ask->levels.strength == i);
		voe_ui_label(ui, STRENGTH_NAMES[i]);
		voe_ui_end(ui);
	}
	voe_ui_end(ui); // Strength row

	voe_ui_row_begin(ui, row);
	voe_ui_label(ui, "Swap");
	for (uint32_t i = 0; i < VOE_EDITOR_LEVELS_ASK_SWAPS; i++) {
		ask->swap_buttons[i] = voe_ui_choice_begin(
			ui, "levels_ask_swap", i, (uint32_t)ask->levels.swap == i);
		voe_ui_label(ui, SWAP_NAMES[i]);
		voe_ui_end(ui);
	}
	voe_ui_end(ui); // Swap row

	voe_ui_row_begin(ui, (voe_ui_container){ .gap = ASK_GAP });
	ask->make_button = voe_ui_button_begin(ui, "levels_ask_make", 0);
	voe_ui_label(ui, "Make");
	voe_ui_end(ui); // Make
	ask->cancel_button = voe_ui_button_begin(ui, "levels_ask_cancel", 0);
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

voe_editor_levels_ask_answer
voe_editor_levels_ask_read(const voe_ui_context *ui, voe_editor_levels_ask *ask,
			   bool pointer_down, voe_math_float2 at)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(ask != NULL, "reading no levels question");

	// A refused frame hands back VOE_UI_NODE_NONE, skipped as assets_ask.c's
	// are; with no panel laid out, no press is outside it either.
	for (uint32_t i = 0; i < VOE_ASSETS_LEVELS; i++)
		if (fired(ui, ask->count_buttons[i]))
			ask->levels.count = i + 1;
	for (uint32_t i = 0; i < VOE_EDITOR_LEVELS_ASK_STRENGTHS; i++)
		if (fired(ui, ask->strength_buttons[i]))
			ask->levels.strength = (voe_assets_levels_strength)i;
	for (uint32_t i = 0; i < VOE_EDITOR_LEVELS_ASK_SWAPS; i++)
		if (fired(ui, ask->swap_buttons[i]))
			ask->levels.swap = (voe_assets_levels_swap)i;
	if (fired(ui, ask->make_button))
		return VOE_EDITOR_LEVELS_ASK_MAKE;
	if (fired(ui, ask->cancel_button))
		return VOE_EDITOR_LEVELS_ASK_CANCEL;
	if (pointer_down && ask->panel != VOE_UI_NODE_NONE &&
	    !voe_editor_inspector_rect_contains(
		    voe_ui_node_visible(ui, ask->panel), at))
		return VOE_EDITOR_LEVELS_ASK_CANCEL;
	return VOE_EDITOR_LEVELS_ASK_NONE;
}

bool voe_editor_levels_ask_save(const voe_editor_levels_ask *ask,
				const char *project_folder,
				voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark;
	voe_assets_parts parts = { 0 };
	voe_assets_material_text text;
	char parts_path[PARTS_PATH];
	const char *full;
	bool ok = false;

	VOE_BASE_ASSERT(ask != NULL && project_folder != NULL,
			"saving the levels of no question");
	VOE_BASE_ASSERT(scratch != NULL, "saving the levels with no scratch");
	VOE_BASE_ASSERT(ask->levels.count >= 1 &&
				ask->levels.count <= VOE_ASSETS_LEVELS,
			"saving a count out of range");

	parts_path_of(ask->path, parts_path);
	mark = voe_base_arena_mark(scratch);
	full = voe_platform_path_join(scratch, project_folder, parts_path);
	// Written over, one that will not read would lose its materials.
	if (voe_platform_file_exists(full) && !parts_loaded(full, scratch, &parts)) {
		fprintf(stderr, "%s: will not read, so its levels are not written\n",
			parts_path);
		goto rewind;
	}
	parts.levels = ask->levels;
	text = voe_assets_parts_write(&parts, scratch);
	ok = voe_platform_file_write(full, (const uint8_t *)text.text, text.size,
				     NULL);
	if (!ok)
		fprintf(stderr, "%s: its levels could not be written\n", parts_path);
rewind:
	voe_base_arena_rewind(scratch, mark);
	return ok;
}
