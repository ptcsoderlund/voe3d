// The Assets panel's right-button menu: its one frame of `ui` calls, the read
// of its rows afterwards, and where it and Create's submenu go next frame. See
// the header for what each holds, the placing rule and who closes it.
//
// Each list is an anchored column round a raised panel of buttons, each button
// holding a row fixed at the widest row's content as measured last frame, as
// panels_menu.c's rows are, so the names start at one left edge.
#include "assets_menu.h"

#include "inspector_place.h"
#include "themes.h"

#include <base/assert.h>

#include <ui/widgets.h>

// Round the rows and between them, as panels_menu.c's list. Millimetres.
#define MENU_PAD (1.0f * VOE_EDITOR_SPACING)

static const char *const ROW_NAMES[VOE_EDITOR_ASSETS_MENU_ROWS] = {
	"Rename", "Duplicate", "Delete"
};
static const voe_editor_assets_menu_item ROW_ITEMS[VOE_EDITOR_ASSETS_MENU_ROWS] = {
	VOE_EDITOR_ASSETS_MENU_RENAME, VOE_EDITOR_ASSETS_MENU_DUPLICATE,
	VOE_EDITOR_ASSETS_MENU_DELETE
};
// Create's kinds, in the order the submenu lists them; a later kind adds its
// name and item here (0377 point 2).
static const char *const KIND_NAMES[VOE_EDITOR_ASSETS_MENU_KINDS] = {
	"Folder", "Landscape"
};
static const voe_editor_assets_menu_item KIND_ITEMS[VOE_EDITOR_ASSETS_MENU_KINDS] = {
	VOE_EDITOR_ASSETS_MENU_FOLDER, VOE_EDITOR_ASSETS_MENU_LANDSCAPE
};
static const char *const CREATE_NAME[1] = { "Create" };

void voe_editor_assets_menu_open(voe_editor_assets_menu *menu,
				 voe_math_float2 at, bool on_row)
{
	VOE_BASE_ASSERT(menu != NULL, "opening no Assets menu");
	*menu = (voe_editor_assets_menu){ .open = true, .on_row = on_row,
					  .at = at, .place = at,
					  .panel = VOE_UI_NODE_NONE,
					  .kinds_panel = VOE_UI_NODE_NONE };
	VOE_BASE_ASSERT(!menu->create_open, "a menu opened with Create's list up");
}

void voe_editor_assets_menu_close(voe_editor_assets_menu *menu)
{
	VOE_BASE_ASSERT(menu != NULL, "closing no Assets menu");
	menu->open = false;
	menu->create_open = false;
}

// One list at `place`: a raised panel keyed `key` of `count` buttons keyed
// `row_key`, each a row `wide` across (natural while nought) holding its name
// and, with `arrow`, the ">" a submenu opens from. Returns the panel.
static voe_ui_node list_draw(voe_ui_context *ui, const char *key,
			     const char *row_key, voe_math_float2 place,
			     float wide, const char *const names[],
			     uint32_t count, bool arrow, voe_ui_node rows[],
			     voe_ui_node inner[])
{
	voe_ui_size row_wide = { VOE_UI_SIZE_NATURAL, 0.0f };
	voe_ui_node panel;

	VOE_BASE_ASSERT(ui != NULL && names != NULL, "drawing no list");
	VOE_BASE_ASSERT(rows != NULL && inner != NULL, "a list with no nodes");
	if (wide > 0.0f)
		row_wide = (voe_ui_size){ VOE_UI_SIZE_FIXED, wide };
	// The column only carries the anchor, as panels_menu.c's.
	voe_ui_column_begin(ui, (voe_ui_container){
					.anchor = { .anchored = true,
						    .x = { VOE_UI_ACROSS_START, place.x },
						    .y = { VOE_UI_ACROSS_START, place.y } } });
	panel = voe_ui_panel_begin(
		ui, key, 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){ .across = VOE_UI_ACROSS_FILL,
				    .gap = MENU_PAD,
				    .pad = { MENU_PAD, MENU_PAD, MENU_PAD, MENU_PAD },
				    .blocks_pointer = true });
	for (uint32_t i = 0; i < count; i++) {
		rows[i] = voe_ui_button_begin(ui, row_key, i);
		inner[i] = voe_ui_row_begin(
			ui, (voe_ui_container){ .size = { .along = row_wide },
						.across = VOE_UI_ACROSS_CENTER,
						.gap = MENU_PAD });
		voe_ui_label(ui, names[i]);
		if (arrow)
			voe_ui_label(ui, ">");
		voe_ui_end(ui); // row
		voe_ui_end(ui); // button
	}
	voe_ui_end(ui); // panel
	voe_ui_end(ui); // anchor column
	return panel;
}

void voe_editor_assets_menu_draw(voe_ui_context *ui, voe_editor_assets_menu *menu)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing the Assets menu into no interface");
	VOE_BASE_ASSERT(menu != NULL && menu->open,
			"drawing an Assets menu that is not open");

	for (uint32_t i = 0; i < VOE_EDITOR_ASSETS_MENU_ROWS; i++)
		menu->rows[i] = menu->names[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_ASSETS_MENU_KINDS; i++)
		menu->kinds[i] = menu->kind_names[i] = VOE_UI_NODE_NONE;
	menu->kinds_panel = VOE_UI_NODE_NONE;
	if (menu->on_row)
		menu->panel = list_draw(ui, "assets menu", "assets menu row",
					menu->place, menu->rows_wide, ROW_NAMES,
					VOE_EDITOR_ASSETS_MENU_ROWS, false,
					menu->rows, menu->names);
	else
		menu->panel = list_draw(ui, "assets menu", "assets menu row",
					menu->place, menu->rows_wide,
					CREATE_NAME, 1, true, menu->rows,
					menu->names);
	if (menu->create_open)
		menu->kinds_panel = list_draw(
			ui, "assets menu create", "assets menu kind",
			menu->kinds_place, menu->kinds_wide, KIND_NAMES,
			VOE_EDITOR_ASSETS_MENU_KINDS, false, menu->kinds,
			menu->kind_names);
}

// Whether `at` is on `panel`'s visible rectangle; no for one not drawn.
static bool list_over(const voe_ui_context *ui, voe_ui_node panel,
		      voe_math_float2 at)
{
	return panel != VOE_UI_NODE_NONE &&
	       voe_editor_inspector_rect_contains(voe_ui_node_visible(ui, panel),
						  at);
}

// The first of `count` buttons that fired, `count` for none. A refused frame
// hands back VOE_UI_NODE_NONE, skipped as topbar.c's buttons are.
static uint32_t list_fired(const voe_ui_context *ui, const voe_ui_node rows[],
			   uint32_t count)
{
	for (uint32_t i = 0; i < count; i++)
		if (rows[i] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, rows[i]).fired)
			return i;
	return count;
}

// The widest row's content: its FIXED width is ignored by the measure
// (layout.h), so a shorter set of rows narrows and nothing ratchets.
static float list_wide(const voe_ui_context *ui, const voe_ui_node inner[],
		       uint32_t count)
{
	float wide = 0.0f;

	for (uint32_t i = 0; i < count; i++)
		if (inner[i] != VOE_UI_NODE_NONE &&
		    voe_ui_node_measured(ui, inner[i]).x > wide)
			wide = voe_ui_node_measured(ui, inner[i]).x;
	return wide;
}

// The menu of `size` at `at`, moved left to the surface's right edge and above
// the pointer when it would leave the surface, then onto it.
static voe_math_float2 menu_fit(voe_math_float2 size, voe_math_float2 at,
				voe_math_float2 surface)
{
	voe_math_float2 place = at;

	if (place.x + size.x > surface.x)
		place.x = surface.x - size.x;
	if (place.y + size.y > surface.y)
		place.y = at.y - size.y;
	if (place.x < 0.0f)
		place.x = 0.0f;
	if (place.y < 0.0f)
		place.y = 0.0f;
	return place;
}

// ADR-0221's rule, as inspector_place.c's submenu: right of `from` when its
// `size` fits before the surface's right edge, else left of it, then wholly
// on the surface; its top at `row`'s, else its bottom at `row`'s bottom.
static voe_math_float2 submenu_fit(voe_ui_rect from, voe_ui_rect row,
				   voe_math_float2 size, voe_math_float2 surface)
{
	voe_math_float2 place = { from.min.x + from.size.x, row.min.y };

	if (place.x + size.x > surface.x)
		place.x = from.min.x - size.x;
	if (place.x + size.x > surface.x)
		place.x = surface.x - size.x;
	if (place.x < 0.0f)
		place.x = 0.0f;
	if (place.y + size.y > surface.y)
		place.y = row.min.y + row.size.y - size.y;
	if (place.y < 0.0f)
		place.y = 0.0f;
	return place;
}

voe_editor_assets_menu_item
voe_editor_assets_menu_read(const voe_ui_context *ui,
			    voe_editor_assets_menu *menu, voe_math_float2 at,
			    bool down, voe_math_float2 surface)
{
	uint32_t count;
	uint32_t row;
	uint32_t kind;
	bool over;
	voe_editor_assets_menu_item fired = VOE_EDITOR_ASSETS_MENU_NONE;

	VOE_BASE_ASSERT(ui != NULL, "reading the Assets menu of no interface");
	VOE_BASE_ASSERT(menu != NULL && menu->open,
			"reading an Assets menu that is not open");
	count = menu->on_row ? VOE_EDITOR_ASSETS_MENU_ROWS : 1;
	over = list_over(ui, menu->panel, at) ||
	       list_over(ui, menu->kinds_panel, at);
	row = list_fired(ui, menu->rows, count);
	kind = list_fired(ui, menu->kinds, VOE_EDITOR_ASSETS_MENU_KINDS);
	if (menu->on_row && row < count)
		fired = ROW_ITEMS[row];
	if (kind < VOE_EDITOR_ASSETS_MENU_KINDS)
		fired = KIND_ITEMS[kind];
	menu->rows_wide = list_wide(ui, menu->names, count);
	menu->kinds_wide =
		list_wide(ui, menu->kind_names, VOE_EDITOR_ASSETS_MENU_KINDS);

	if (menu->panel != VOE_UI_NODE_NONE) {
		voe_ui_rect panel = voe_ui_node_rect(ui, menu->panel);
		voe_math_float2 kinds_size = { 0.0f, 0.0f };

		menu->place = menu_fit(panel.size, menu->at, surface);
		if (menu->kinds_panel != VOE_UI_NODE_NONE)
			kinds_size = voe_ui_node_rect(ui, menu->kinds_panel).size;
		if (!menu->on_row && menu->rows[0] != VOE_UI_NODE_NONE)
			menu->kinds_place = submenu_fit(
				panel, voe_ui_node_rect(ui, menu->rows[0]),
				kinds_size, surface);
	}
	if (!menu->on_row && row == 0)
		menu->create_open = !menu->create_open;
	if (fired != VOE_EDITOR_ASSETS_MENU_NONE || (down && !over))
		voe_editor_assets_menu_close(menu);
	VOE_BASE_ASSERT(fired <= VOE_EDITOR_ASSETS_MENU_LANDSCAPE,
			"an Assets menu row past the items");
	return fired;
}
