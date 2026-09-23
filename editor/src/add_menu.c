// Add component's tree built from the world's types: each offered type's path
// split into trimmed parts, each group part found or made under its parent,
// and the entry appended last; and its top level drawn as a list the way
// inspector.c's dropdown_list draws the open dropdown's. See the header for
// what is offered and why.
#include "add_menu.h"

#include "inspector_value.h"

#include <base/assert.h>

#include <ui/widgets.h>

#include <stddef.h>
#include <string.h>

// Round the list's rows and between them, and the right padding a capped list
// keeps for its Y scrollbar: inspector.c's LIST_PAD and LIST_BAR, so the two
// lists look alike. Millimetres.
#define LIST_PAD 1.0f
#define LIST_BAR 3.5f

// Copies the trimmed part of `path` starting at `*at` into `label` and moves
// `*at` past it and its `/`. True when that part was the last one.
static bool next_part(const char *path, size_t *at,
		      char label[VOE_EDITOR_ADD_MENU_LABEL])
{
	size_t start = *at;
	size_t end;
	size_t length;
	bool last;

	VOE_BASE_ASSERT(path != NULL, "splitting no menu path");
	VOE_BASE_ASSERT(label != NULL, "splitting a menu path into no label");

	end = start + strcspn(path + start, "/");
	last = path[end] == '\0';
	*at = last ? end : end + 1;
	while (start < end && path[start] == ' ')
		start++;
	while (end > start && path[end - 1] == ' ')
		end--;
	length = end - start;
	VOE_BASE_ASSERT(length > 0, "a menu path has an empty part");
	VOE_BASE_ASSERT(length < VOE_EDITOR_ADD_MENU_LABEL,
			"a menu path part is longer than a label");
	memcpy(label, path + start, length);
	label[length] = '\0';
	return last;
}

// The index of a new entry at the end of `menu`, its label and parent set.
static uint32_t append(voe_editor_add_menu *menu, const char *label,
		       uint32_t parent, bool group)
{
	voe_editor_add_menu_entry *entry;

	VOE_BASE_ASSERT(menu->count < VOE_EDITOR_ADD_MENU_ENTRIES,
			"Add component has more entries than it can hold");
	entry = &menu->entries[menu->count];
	*entry = (voe_editor_add_menu_entry){ .parent = parent, .group = group };
	memcpy(entry->label, label, VOE_EDITOR_ADD_MENU_LABEL);
	VOE_BASE_ASSERT(entry->label[VOE_EDITOR_ADD_MENU_LABEL - 1] == '\0',
			"a menu label lost its zero");
	return menu->count++;
}

// The group labelled `label` under `parent`, made when there is none yet.
static uint32_t group_under(voe_editor_add_menu *menu, const char *label,
			    uint32_t parent)
{
	VOE_BASE_ASSERT(menu != NULL, "finding a group in no menu");
	VOE_BASE_ASSERT(label != NULL, "finding a group with no label");

	for (uint32_t i = 0; i < menu->count; i++) {
		const voe_editor_add_menu_entry *entry = &menu->entries[i];

		if (entry->group && entry->parent == parent &&
		    strcmp(entry->label, label) == 0)
			return i;
	}
	return append(menu, label, parent, true);
}

static bool offered(const voe_ecs_world *world, voe_ecs_type type,
		    voe_ecs_entity entity)
{
	return !voe_ecs_component_runtime_only(world, type) &&
	       voe_ecs_component_default(world, type) != NULL &&
	       voe_ecs_component_menu(world, type) != NULL &&
	       voe_ecs_component_get(world, type, entity) == NULL;
}

// Every part of `path` but the last found or made as a group, then the last
// appended as the entry for `type`.
static void add_path(voe_editor_add_menu *menu, const char *path,
		     voe_ecs_type type)
{
	char label[VOE_EDITOR_ADD_MENU_LABEL] = { 0 };
	uint32_t parent = VOE_EDITOR_ADD_MENU_TOP;
	size_t at = 0;

	for (uint32_t depth = 0; depth < VOE_EDITOR_ADD_MENU_DEPTH; depth++) {
		if (next_part(path, &at, label)) {
			uint32_t index = append(menu, label, parent, false);

			menu->entries[index].type = type;
			return;
		}
		parent = group_under(menu, label, parent);
	}
	VOE_BASE_ASSERT(false, "a menu path is deeper than Add component goes");
}

void voe_editor_add_menu_build(voe_editor_add_menu *menu,
			       const voe_ecs_world *world,
			       voe_ecs_entity entity)
{
	uint32_t types;

	VOE_BASE_ASSERT(menu != NULL, "building Add component into no menu");
	VOE_BASE_ASSERT(world != NULL, "building Add component from no world");

	menu->count = 0;
	types = voe_ecs_component_type_count(world);
	for (uint32_t i = 0; i < types; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (offered(world, type, entity))
			add_path(menu, voe_ecs_component_menu(world, type),
				 type);
	}
	VOE_BASE_ASSERT(menu->count <= VOE_EDITOR_ADD_MENU_ENTRIES,
			"Add component overran its entries");
}

// The column round the list only carries the anchor, the list being a panel of
// its own; the rows scroll inside it so a capped list still reaches its last
// entry (ADR-0200). A type's label is the menu's own, which the caller keeps
// past voe_ui_frame_end; a group's, with its marker, is the arena's.
void voe_editor_add_menu_draw(voe_ui_context *ui, voe_base_arena *arena,
			      const voe_editor_add_menu *menu, float left,
			      float top, float height,
			      voe_editor_add_menu_list *list)
{
	const bool capped = height > 0.0f;

	VOE_BASE_ASSERT(ui != NULL, "drawing Add component into no interface");
	VOE_BASE_ASSERT(menu != NULL && list != NULL,
			"drawing no Add component or into no list");

	list->row_count = 0;
	voe_ui_column_begin(
		ui, (voe_ui_container){
			    .anchor = { .anchored = true,
					.x = { VOE_UI_ACROSS_START, left },
					.y = { VOE_UI_ACROSS_START, top } } });
	list->panel = voe_ui_panel_begin(
		ui, "add menu", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){ .across = VOE_UI_ACROSS_FILL,
				    .pad = { LIST_PAD, LIST_PAD, LIST_PAD,
					     LIST_PAD },
				    .blocks_pointer = true });
	list->area = voe_ui_scroll_begin(
		ui, "add menu rows", 0,
		(voe_ui_container){
			.size = { .along = capped
					   ? (voe_ui_size){ VOE_UI_SIZE_FIXED,
							    height }
					   : (voe_ui_size){ 0 } },
			.across = VOE_UI_ACROSS_FILL,
			.gap = LIST_PAD,
			.pad = { .right = capped ? LIST_BAR : 0.0f } },
		(voe_ui_scroll_axes){ .y = true });
	for (uint32_t i = 0; i < menu->count; i++) {
		const voe_editor_add_menu_entry *entry = &menu->entries[i];

		if (entry->parent != VOE_EDITOR_ADD_MENU_TOP)
			continue;
		list->rows[list->row_count++] = (voe_editor_add_menu_row){
			.node = voe_ui_button_begin(ui, "add menu row", i),
			.entry = i
		};
		voe_ui_label(ui, entry->group ? text(arena, "%s >",
						     entry->label)
					      : entry->label);
		voe_ui_end(ui);
	}
	voe_ui_end(ui);
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_BASE_ASSERT(list->row_count <= menu->count,
			"Add component drew more rows than it has entries");
}
