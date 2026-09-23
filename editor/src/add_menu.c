// Add component's tree built from the world's types: each offered type's path
// split into trimmed parts, each group part found or made under its parent,
// and the entry appended last. See the header for what is offered and why.
#include "add_menu.h"

#include <base/assert.h>

#include <stddef.h>
#include <string.h>

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
