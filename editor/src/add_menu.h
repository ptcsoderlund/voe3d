// The entries Add component offers one entity, as a tree: groups and the types
// under them. Built every frame from the world's types, and each level drawn as
// one of the Inspector's open lists.
//
//     voe_editor_add_menu_build(&menu, world, entity);
//     voe_editor_add_menu_draw(ui, arena, &menu, parent, place, &list);
//     ...     // after voe_ui_frame_end, list.rows[i] fired: entry list.rows[i].entry
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
// A LIST IS DRAWN AS THE OPEN DROPDOWN'S IS (inspector.h, ADR-0199, 0200): an
// anchored column at `left`, `top` in the Inspector's content column, emitted
// after every section, a panel that takes the pointer, and its rows in a scroll
// area capped to `height` when that is not nought. One row per entry whose
// parent is the one asked for; a group's label is followed by " >", the marker
// that it opens further. The top list is drawn with VOE_EDITOR_ADD_MENU_TOP,
// then one list per open group with that group's children, each later than the
// one it opened from so it paints over it.
//
// SUBMENU PLACEMENT (ADR-0221) is the caller's: beside its group row, to the
// right of the list it opened from, on the left when the panel's area has no
// room on the right, fitted vertically by ADR-0200's rule (inspector_edit.h).
// EVERY OPEN LIST IS SOLID OVER ITS OUTLINE (ADR-0199): each panel blocks the
// pointer, so nothing under any of them hovers or takes a click.
//
// CONSTRAINTS. A path is the program's own, never read from a file, so an empty
// part, a path deeper than VOE_EDITOR_ADD_MENU_DEPTH, a part longer than
// VOE_EDITOR_ADD_MENU_LABEL or more than VOE_EDITOR_ADD_MENU_ENTRIES entries
// asserts. Finding a group is a scan of the entries built so far, which the
// entry cap keeps cheap.
#pragma once

#include <base/arena.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <game/project.h>
#include <game/world.h>

#include <ui/layout.h>

#include <stdbool.h>
#include <stdint.h>

// Levels of a path, the entry's included.
#define VOE_EDITOR_ADD_MENU_DEPTH 4
// Every engine and project type (game/world.h, game/project.h) and the folder
// entries their menu paths open: at most a path's depth apiece, groups shared
// or not, so no project's paths can reach the assert.
#define VOE_EDITOR_ADD_MENU_ENTRIES \
	((VOE_GAME_WORLD_TYPES + VOE_GAME_PROJECT_TYPES) * \
	 VOE_EDITOR_ADD_MENU_DEPTH)
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

// One drawn row of the list: its button, and the index of the entry it shows.
typedef struct {
	voe_ui_node node;
	uint32_t entry;
} voe_editor_add_menu_row;

// The list as drawn this frame, for the read after voe_ui_frame_end. `panel` and
// `area` are VOE_UI_NODE_NONE when no list was drawn.
typedef struct {
	voe_ui_node panel;
	voe_ui_node area;
	voe_editor_add_menu_row rows[VOE_EDITOR_ADD_MENU_ENTRIES];
	uint32_t row_count;
} voe_editor_add_menu_list;

// Clears `menu` and fills it with what `entity` can be given.
void voe_editor_add_menu_build(voe_editor_add_menu *menu,
			       const voe_ecs_world *world,
			       voe_ecs_entity entity);

// Draws the children of `parent` in `menu` into `list`, which it clears first;
// VOE_EDITOR_ADD_MENU_TOP is the top level. A group's label with its marker is
// formatted into `arena`, the frame's, because a label is read at
// voe_ui_frame_end.
void voe_editor_add_menu_draw(voe_ui_context *ui, voe_base_arena *arena,
			      const voe_editor_add_menu *menu, uint32_t parent,
			      float left, float top, float height,
			      voe_editor_add_menu_list *list);
