// The Panels list's one frame of `ui` calls and the read of its rows
// afterwards. See the header for why the tick is √ and who closes the list.
//
// Each row is a button holding a row that grows to the button's width, so its
// content starts at the left rather than centred, and in it the tick column at
// a fixed width and the panel's name.
#include "panels_menu.h"

#include "inspector_place.h"
#include "themes.h"

#include <base/assert.h>

#include <ui/widgets.h>

// Round the rows and between them, and between the tick and the name, as
// add_menu.c's lists are padded. Millimetres.
#define MENU_PAD (1.0f * VOE_EDITOR_SPACING)
// The tick column's width. Millimetres.
#define TICK_WIDE (6.0f * VOE_EDITOR_SPACING)

void voe_editor_panels_menu_draw(voe_ui_context *ui, voe_editor_panels_menu *menu,
				 voe_ui_rect under,
				 const bool open[VOE_EDITOR_CLOSABLE_COUNT])
{
	VOE_BASE_ASSERT(ui != NULL, "drawing the Panels list into no interface");
	VOE_BASE_ASSERT(menu != NULL && open != NULL,
			"drawing no Panels list or with no open flags");
	VOE_BASE_ASSERT(menu->open, "drawing a Panels list that is not open");

	// The column round the panel only carries the anchor, as add_menu.c's.
	voe_ui_column_begin(
		ui, (voe_ui_container){
			    .anchor = { .anchored = true,
					.x = { VOE_UI_ACROSS_START, under.min.x },
					.y = { VOE_UI_ACROSS_START,
					       under.min.y + under.size.y } } });
	menu->panel = voe_ui_panel_begin(
		ui, "panels menu", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){ .across = VOE_UI_ACROSS_FILL,
				    .gap = MENU_PAD,
				    .pad = { MENU_PAD, MENU_PAD, MENU_PAD,
					     MENU_PAD },
				    .blocks_pointer = true });
	for (uint32_t i = 0; i < VOE_EDITOR_CLOSABLE_COUNT; i++) {
		menu->rows[i] = voe_ui_button_begin(ui, "panels menu row", i);
		voe_ui_row_begin(ui, (voe_ui_container){
					     .size = { .along = { VOE_UI_SIZE_GROW,
								  1.0f } },
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = MENU_PAD });
		voe_ui_column_begin(ui, (voe_ui_container){
						.size = { .along = { VOE_UI_SIZE_FIXED,
								     TICK_WIDE } },
						.across = VOE_UI_ACROSS_CENTER });
		if (open[i])
			voe_ui_label(ui, "√");
		voe_ui_end(ui); // tick column
		voe_ui_label(ui, voe_editor_closable_name((voe_editor_closable)i));
		voe_ui_end(ui); // row
		voe_ui_end(ui); // button
	}
	voe_ui_end(ui); // panel
	voe_ui_end(ui); // anchor column
}

voe_editor_closable voe_editor_panels_menu_read(const voe_ui_context *ui,
						const voe_editor_panels_menu *menu,
						voe_math_float2 at, bool *over)
{
	voe_editor_closable fired = VOE_EDITOR_CLOSABLE_COUNT;

	VOE_BASE_ASSERT(ui != NULL, "reading the Panels list of no interface");
	VOE_BASE_ASSERT(menu != NULL && over != NULL,
			"reading no Panels list or into nothing");

	// A refused frame hands back VOE_UI_NODE_NONE past the node budget,
	// skipped as topbar.c's buttons are.
	*over = menu->panel != VOE_UI_NODE_NONE &&
		voe_editor_inspector_rect_contains(
			voe_ui_node_visible(ui, menu->panel), at);
	for (uint32_t i = 0; i < VOE_EDITOR_CLOSABLE_COUNT; i++)
		if (fired == VOE_EDITOR_CLOSABLE_COUNT &&
		    menu->rows[i] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, menu->rows[i]).fired)
			fired = (voe_editor_closable)i;

	VOE_BASE_ASSERT(fired <= VOE_EDITOR_CLOSABLE_COUNT,
			"a Panels row past the closables");
	return fired;
}
