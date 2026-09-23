// The entries Add component offers one entity, as a tree: groups and the types
// under them. Built every frame from the world's types and drawn by the
// Inspector; nothing in here draws.
//
//     voe_editor_add_menu menu;
//     voe_editor_add_menu_build(&menu, world, entity);
//     for (uint32_t i = 0; i < menu.count; i++)
//             ...     // menu.entries[i], its parent an index before i
//
// THE MENU IS BUILT FROM voe_ecs_component_menu (ecs/include/ecs/component.h),
// the path each declaring folder registered, so the editor names no component
// (ADR-0217) and the menu is never a hand-kept list.
//
// A TYPE IS OFFERED WHEN it is described (not runtime-only), has a default row
// and a menu path, and the entity lacks it (ADR-0221). A type with no path is
// never offered, which is how identity and the camera stay out.
//
// A PATH SPLITS ON `/`, each part trimmed of the spaces round it: the last part
// is the entry, the ones before it groups. "Rendering / Light" and
// "Rendering / Shape" share one "Rendering", found by label under the same
// parent before a new group is made.
//
// ORDER: groups come in the order their first entry's type was registered,
// entries in registration order. A group exists only if it holds an offered
// entry. A parent always sits before its children in the array.
//
// CONSTRAINTS. A path is the program's own, never read from a file, so an empty
// part, a path deeper than VOE_EDITOR_ADD_MENU_DEPTH, a part longer than
// VOE_EDITOR_ADD_MENU_LABEL or more than VOE_EDITOR_ADD_MENU_ENTRIES entries
// asserts. Finding a group is a scan of the entries built so far, which the
// entry cap keeps cheap.
#pragma once

#include <ecs/component.h>
#include <ecs/world.h>

#include <stdbool.h>
#include <stdint.h>

// Levels of a path, the entry's included.
#define VOE_EDITOR_ADD_MENU_DEPTH 4
#define VOE_EDITOR_ADD_MENU_ENTRIES 32
// Bytes of one part's label, its zero included.
#define VOE_EDITOR_ADD_MENU_LABEL 32
// The parent of a top-level entry.
#define VOE_EDITOR_ADD_MENU_TOP UINT32_MAX

// A group or an offered type. `type` means something only when `group` is
// false; `parent` is an index into the same array or VOE_EDITOR_ADD_MENU_TOP.
typedef struct {
	char label[VOE_EDITOR_ADD_MENU_LABEL];
	uint32_t parent;
	bool group;
	voe_ecs_type type;
} voe_editor_add_menu_entry;

typedef struct {
	voe_editor_add_menu_entry entries[VOE_EDITOR_ADD_MENU_ENTRIES];
	uint32_t count;
} voe_editor_add_menu;

// Clears `menu` and fills it with what `entity` can be given.
void voe_editor_add_menu_build(voe_editor_add_menu *menu,
			       const voe_ecs_world *world,
			       voe_ecs_entity entity);
