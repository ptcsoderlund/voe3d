// The levels question's open with its one quiet read of the `.parts` and
// count of the triangles, its one frame of `ui` calls with the chosen count
// lit as inspector_sculpt.c lights its chosen kind and a row per
// LOD of its name, share, swap and triangles, the read of its buttons, its
// share and swap boxes and a press outside it afterwards, and Make's save into
// the `.parts`. See the header for what is shown and what a save refuses.
#include "levels_ask.h"

#include "inspector_place.h"
#include "themes.h"

#include <base/assert.h>
#include <game/models.h>
#include <platform/file.h>
#include <platform/path.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

static_assert(VOE_ASSETS_LEVELS == VOE_3D_MODEL_LEVELS,
	      "the panel's levels and the counted ones differ");

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
	for (uint32_t i = 0; i < VOE_ASSETS_LEVELS; i++) {
		ask->count_buttons[i] = VOE_UI_NODE_NONE;
		ask->share_boxes[i] = VOE_UI_NODE_NONE;
		ask->swap_boxes[i] = VOE_UI_NODE_NONE;
	}
	snprintf(ask->path, sizeof ask->path, "%s", path);
	name = strrchr(ask->path, '/');
	name = name != NULL ? name + 1 : ask->path;
	snprintf(ask->title, sizeof ask->title, "LODs of %s", name);

	parts_path_of(ask->path, parts_path);
	mark = voe_base_arena_mark(scratch);
	full = voe_platform_path_join(scratch, project_folder, parts_path);
	if (voe_platform_file_exists(full) && parts_loaded(full, scratch, &there))
		ask->levels = there.levels;
	ask->opened = ask->levels;
	ask->known = voe_game_models_levels_triangles(project_folder, ask->path,
						      scratch, ask->triangles,
						      &ask->own, &ask->fresh);
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(ask->levels.count >= 1 &&
				ask->levels.count <= VOE_ASSETS_LEVELS,
			"a levels question opened with a count out of range");
	VOE_BASE_ASSERT(ask->known <= VOE_ASSETS_LEVELS,
			"more levels counted than a model has");
}

void voe_editor_levels_ask_close(voe_editor_levels_ask *ask)
{
	VOE_BASE_ASSERT(ask != NULL, "closing no levels question");

	ask->open = false;
	VOE_BASE_ASSERT(!ask->open, "a levels question left open");
}

// LOD k + 1's row: its name, its share as a number box or, on own levels, a
// label of its triangles against LOD 1's, its swap box and its triangles.
static void draw_level_row(voe_ui_context *ui, voe_editor_levels_ask *ask,
			   uint32_t k, voe_ui_container row)
{
	const voe_assets_levels *levels = &ask->levels;

	VOE_BASE_ASSERT(ui != NULL && ask != NULL, "drawing no level row");
	VOE_BASE_ASSERT(k >= 1 && k < VOE_ASSETS_LEVELS,
			"drawing a level row out of range");

	snprintf(ask->row_names[k], sizeof ask->row_names[k], "LOD %u",
		 (unsigned)(k + 1));
	if (k < ask->known)
		snprintf(ask->row_triangles[k], sizeof ask->row_triangles[k],
			 "%u", (unsigned)ask->triangles[k]);
	else
		snprintf(ask->row_triangles[k], sizeof ask->row_triangles[k],
			 "—");
	voe_ui_row_begin(ui, row);
	voe_ui_label(ui, ask->row_names[k]);
	if (ask->own) {
		if (ask->triangles[0] > 0)
			snprintf(ask->row_shares[k], sizeof ask->row_shares[k],
				 "%g %%",
				 round(ask->triangles[k] * 1000.0 /
				       ask->triangles[0]) / 10.0);
		else
			snprintf(ask->row_shares[k], sizeof ask->row_shares[k],
				 "—");
		voe_ui_label(ui, ask->row_shares[k]);
	} else {
		snprintf(ask->row_shares[k], sizeof ask->row_shares[k], "%g %%",
			 (double)levels->shares[k]);
		ask->share_boxes[k] = voe_ui_number_begin(ui, "share", k,
							  levels->shares[k], 1.0);
		voe_ui_label(ui, ask->row_shares[k]);
		voe_ui_end(ui); // share
	}
	snprintf(ask->row_swaps[k], sizeof ask->row_swaps[k], "×%g",
		 (double)levels->swaps[k]);
	ask->swap_boxes[k] = voe_ui_number_begin(ui, "swap", k, levels->swaps[k],
						 0.1);
	voe_ui_label(ui, ask->row_swaps[k]);
	voe_ui_end(ui); // swap
	voe_ui_label(ui, ask->row_triangles[k]);
	voe_ui_end(ui); // LOD row
	VOE_BASE_ASSERT(ask->row_names[k][0] != '\0' &&
				ask->row_triangles[k][0] != '\0',
			"a level row drawn with no name or triangles");
}

void voe_editor_levels_ask_draw(voe_ui_context *ui, voe_editor_levels_ask *ask,
				float top)
{
	const voe_ui_container row = { .across = VOE_UI_ACROSS_CENTER,
				       .gap = ASK_GAP };
	uint32_t rows;

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

	if (!ask->own) {
		voe_ui_row_begin(ui, row);
		voe_ui_label(ui, "LODs");
		for (uint32_t i = 0; i < VOE_ASSETS_LEVELS; i++) {
			ask->count_buttons[i] = voe_ui_choice_begin(
				ui, "levels_ask_count", i,
				ask->levels.count == i + 1);
			voe_ui_label(ui, COUNT_NAMES[i]);
			voe_ui_end(ui);
		}
		voe_ui_end(ui); // LODs row
	}

	// A row not drawn this frame, the count lowered, is read as none.
	for (uint32_t k = 0; k < VOE_ASSETS_LEVELS; k++) {
		ask->share_boxes[k] = VOE_UI_NODE_NONE;
		ask->swap_boxes[k] = VOE_UI_NODE_NONE;
	}
	rows = ask->own ? ask->known : ask->levels.count;
	for (uint32_t k = 1; k < rows; k++)
		draw_level_row(ui, ask, k, row);

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

// Whether the number box `node` was laid out this frame and changed, its value
// into `value` when it did.
static bool box_changed(const voe_ui_context *ui, voe_ui_node node,
			double *value)
{
	voe_ui_number_result result;

	VOE_BASE_ASSERT(ui != NULL && value != NULL, "reading a box of nothing");
	if (node == VOE_UI_NODE_NONE)
		return false;
	result = voe_ui_number_action(ui, node);
	*value = result.value;
	return result.changed;
}

// `value` rounded to a tenth.
static float tenth(double value)
{
	return (float)(round(value * 10.0) / 10.0);
}

// Share `k` set to `value` in tenths, at least a tenth for it and each LOD
// after it and a tenth below the share before; each later share at or above
// the one before it pushed a tenth below that one. Left as it is when the
// share before leaves no such room, which only a hand-written `.parts` does:
// a save asserts the shares in order.
static void share_set(voe_assets_levels *levels, uint32_t k, double value)
{
	const float least = 0.1f * (float)(VOE_ASSETS_LEVELS - k);
	float most;

	VOE_BASE_ASSERT(levels != NULL, "setting a share of no levels");
	VOE_BASE_ASSERT(k >= 1 && k < VOE_ASSETS_LEVELS,
			"setting a share out of range");
	most = levels->shares[k - 1] - 0.1f;
	if (most < least)
		return;
	levels->shares[k] = fminf(fmaxf(tenth(value), least), most);
	for (uint32_t j = k + 1; j < VOE_ASSETS_LEVELS; j++)
		if (levels->shares[j] >= levels->shares[j - 1])
			levels->shares[j] = levels->shares[j - 1] - 0.1f;
	VOE_BASE_ASSERT(levels->shares[VOE_ASSETS_LEVELS - 1] > 0.0f,
			"a share pushed down to nothing");
}

voe_editor_levels_ask_answer
voe_editor_levels_ask_read(const voe_ui_context *ui, voe_editor_levels_ask *ask,
			   bool pointer_down, voe_math_float2 at)
{
	double value;

	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(ask != NULL, "reading no levels question");

	// A refused frame hands back VOE_UI_NODE_NONE, skipped as assets_ask.c's
	// are; with no panel laid out, no press is outside it either.
	for (uint32_t i = 0; i < VOE_ASSETS_LEVELS; i++)
		if (fired(ui, ask->count_buttons[i]))
			ask->levels.count = i + 1;
	for (uint32_t k = 1; k < VOE_ASSETS_LEVELS; k++) {
		if (box_changed(ui, ask->share_boxes[k], &value))
			share_set(&ask->levels, k, value);
		if (box_changed(ui, ask->swap_boxes[k], &value))
			ask->levels.swaps[k] =
				fminf(fmaxf(tenth(value),
					    VOE_ASSETS_LEVELS_SWAP_LEAST),
				      VOE_ASSETS_LEVELS_SWAP_MOST);
	}
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

// Own levels are not cut by the shares, so only their swaps can differ; made
// ones keep their file while the count and every share are unchanged.
bool voe_editor_levels_ask_swaps_only(const voe_editor_levels_ask *ask)
{
	const voe_assets_levels *now;
	const voe_assets_levels *was;
	bool same;

	VOE_BASE_ASSERT(ask != NULL, "asking whether no question changed swaps");
	if (!ask->fresh)
		return false;
	if (ask->own)
		return true;
	now = &ask->levels;
	was = &ask->opened;
	same = now->count == was->count;
	for (uint32_t k = 1; same && k < VOE_ASSETS_LEVELS; k++)
		same = now->shares[k] == was->shares[k];
	return same;
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
