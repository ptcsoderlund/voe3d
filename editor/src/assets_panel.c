// The Assets panel's listing with the selected row kept, its one frame of `ui`
// calls with the naming's field, and the read of its rows, the selection and
// `keyboard`, the field's request, a fired prefab row's path and Up afterwards,
// with the rectangles the right button's menu and the drag are read from.
// See the header for the arena, the once a second and what a missing
// `Assets/` or a failed listing leaves.
#include "assets_panel.h"

#include "inspector_place.h"
#include "themes.h"

#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// Block sizes, not limits, as browser.c's are.
#define ASSETS_ARENA (16u * 1024u)
#define ASSETS_SCRATCH (64u * 1024u)
#define ASSETS_FOLDER "Assets"
#define LIST_SECONDS 1.0
#define MISSING_LINE "No Assets folder in this project."
#define ASSETS_GAP (2.0f * VOE_EDITOR_SPACING)
#define NEW_FOLDER_NAME "New folder"
#define NEW_LANDSCAPE_NAME "New landscape"

static char *copy_string(voe_base_arena *arena, const char *text)
{
	size_t size;
	char *copy;

	VOE_BASE_ASSERT(arena != NULL && text != NULL, "copying into no arena");
	size = strlen(text) + 1;
	copy = voe_base_arena_push(arena, size);

	VOE_BASE_ASSERT(copy != NULL, "out of memory copying an asset's name");
	memcpy(copy, text, size);
	return copy;
}

static bool same_folder(const char *a, const char *b)
{
	return (a == NULL && b == NULL) ||
	       (a != NULL && b != NULL && strcmp(a, b) == 0);
}

static bool listed_into(const char *path, voe_base_arena *scratch,
			voe_platform_folder_listing *listing)
{
	VOE_BASE_ASSERT(path != NULL && scratch != NULL, "listing no folder");
	VOE_BASE_ASSERT(listing != NULL, "listing a folder into nowhere");
	voe_base_report_error_clear();
	return voe_platform_folder_list(path, scratch, listing, NULL);
}

// `shown` cut at its last separator, in place; "" for a top-level folder.
static void cut_to_parent(char *shown)
{
	char *last = NULL;

	VOE_BASE_ASSERT(shown != NULL, "cutting no folder to its parent");
	for (char *c = shown; *c != '\0'; c++)
		if (*c == '/' || *c == '\\')
			last = c;
	if (last != NULL)
		*last = '\0';
	else
		shown[0] = '\0';
}

// Lists `<project>/Assets/<shown>` into `listing`. False on a failed listing;
// true with `missing` set when there is no project or no `Assets/`, whose
// presence is read from the project's own listing so it reports nothing.
static bool list_shown(const char *project, const char *shown,
		       voe_base_arena *scratch,
		       voe_platform_folder_listing *listing, bool *missing)
{
	voe_platform_folder_listing top;
	const char *path;

	VOE_BASE_ASSERT(shown != NULL && scratch != NULL, "listing no shown folder");
	VOE_BASE_ASSERT(listing != NULL && missing != NULL, "listing into nowhere");
	*missing = true;
	*listing = (voe_platform_folder_listing){ 0 };
	if (project == NULL)
		return true;
	if (!listed_into(project, scratch, &top))
		return false;
	for (uint32_t i = 0; i < top.count; i++)
		if (top.entries[i].folder &&
		    strcmp(top.entries[i].name, ASSETS_FOLDER) == 0)
			*missing = false;
	if (*missing)
		return true;
	path = voe_platform_path_join(scratch, project, ASSETS_FOLDER);
	if (shown[0] != '\0')
		path = voe_platform_path_join(scratch, path, shown);
	return listed_into(path, scratch, listing);
}

// Whether `name` ends `ending`, lower case, in any case.
static bool name_ends(const char *name, const char *ending)
{
	size_t length;
	size_t tail;

	VOE_BASE_ASSERT(name != NULL && ending != NULL,
			"asking whether no name ends in something");
	length = strlen(name);
	tail = strlen(ending);
	if (length < tail)
		return false;
	name += length - tail;
	for (size_t i = 0; i < tail; i++)
		if (tolower((unsigned char)name[i]) != ending[i])
			return false;
	return true;
}

// Whether `name` ends `.png`, `.jpg` or `.jpeg`, in any case.
static bool names_picture(const char *name)
{
	VOE_BASE_ASSERT(name != NULL, "asking whether no name is a picture");
	return name_ends(name, ".png") || name_ends(name, ".jpg") ||
	       name_ends(name, ".jpeg");
}

// Folders in the first pass, files in the second, each in the listing's name
// order, hidden ones left out, up to VOE_EDITOR_BROWSER_ROWS.
static void rows_fill(voe_editor_assets *assets,
		      const voe_platform_folder_listing *listing)
{
	VOE_BASE_ASSERT(assets != NULL && assets->arena != NULL, "filling no rows");
	VOE_BASE_ASSERT(listing != NULL, "filling rows from no listing");
	assets->row_count = 0;
	for (uint32_t pass = 0; pass < 2; pass++) {
		for (uint32_t i = 0; i < listing->count &&
				     assets->row_count < VOE_EDITOR_BROWSER_ROWS;
		     i++) {
			const voe_platform_folder_entry *e = &listing->entries[i];

			if (e->hidden || e->folder != (pass == 0))
				continue;
			assets->rows[assets->row_count++] = (voe_editor_assets_row){
				.node = VOE_UI_NODE_NONE,
				.name = copy_string(assets->arena, e->name),
				.folder = e->folder,
				// A landscape is worn as a model (0379 point 2).
				.model = !e->folder &&
					 (voe_editor_browser_names_model(e->name) ||
					  name_ends(e->name, ".landscape")),
				.prefab = !e->folder &&
					  name_ends(e->name, ".prefab"),
				.picture = !e->folder && names_picture(e->name),
			};
		}
	}
}

// The selected row found again among the new rows by `name`, NULL when gone
// or when `name` is; a rename whose row is gone ends.
static void selected_find(voe_editor_assets *assets, const char *name)
{
	VOE_BASE_ASSERT(assets != NULL, "finding a selection in no panel");
	assets->selected = NULL;
	for (uint32_t i = 0; i < assets->row_count && name != NULL; i++)
		if (strcmp(assets->rows[i].name, name) == 0)
			assets->selected = assets->rows[i].name;
	if (assets->selected == NULL &&
	    assets->naming == VOE_EDITOR_ASSETS_NAMING_RENAME)
		assets->naming = VOE_EDITOR_ASSETS_NAMING_NONE;
	VOE_BASE_ASSERT(assets->selected == NULL || name != NULL,
			"a selection found for no name");
}

// Lists `project`'s `shown`, entered into `name` when it is not NULL or up a
// level when `up`, and commits it to the arena. Both strings are copied into
// scratch first: either may point into the arena being cleared. The selected
// row and the naming are kept only when the same folder is listed again.
static void relist(voe_editor_assets *assets, const char *project,
		   const char *shown, const char *name, bool up)
{
	voe_base_arena *scratch;
	voe_platform_folder_listing listing;
	const char *kept;
	const char *selected = NULL;
	char *next;
	bool missing;
	bool same;

	VOE_BASE_ASSERT(assets != NULL && assets->arena != NULL, "listing no panel");
	VOE_BASE_ASSERT(shown != NULL, "listing no shown folder");
	scratch = voe_base_arena_new(ASSETS_SCRATCH);
	kept = project ? copy_string(scratch, project) : NULL;
	next = copy_string(scratch, shown);
	same = assets->listed_once && same_folder(kept, assets->project);
	if (same && name == NULL && !up && strcmp(next, assets->shown) == 0 &&
	    assets->selected != NULL)
		selected = copy_string(scratch, assets->selected);
	if (!same || name != NULL || up)
		assets->naming = VOE_EDITOR_ASSETS_NAMING_NONE;

	if (name != NULL)
		next = next[0] == '\0' ? copy_string(scratch, name) :
			(char *)voe_platform_path_join(scratch, next, name);
	else if (up)
		cut_to_parent(next);

	if (!list_shown(kept, next, scratch, &listing, &missing) && same) {
		voe_base_arena_destroy(scratch);
		return;
	}
	voe_base_arena_clear(assets->arena);
	assets->held = NULL;
	assets->project =kept ? copy_string(assets->arena, kept) : NULL;
	assets->shown = copy_string(assets->arena, next);
	assets->path = next[0] == '\0' ?
		ASSETS_FOLDER "/" :
		voe_platform_path_join(assets->arena, ASSETS_FOLDER, next);
	assets->missing = missing;
	assets->listed_once = true;
	rows_fill(assets, &listing);
	selected_find(assets, selected);
	voe_base_arena_destroy(scratch);
}

void voe_editor_assets_update(voe_editor_assets *assets,
			      const char *project_folder, double now)
{
	VOE_BASE_ASSERT(assets != NULL, "updating no Assets panel");
	VOE_BASE_ASSERT(now >= 0.0, "a frame clock before its own start");

	// A zeroed panel's buttons are node 0, not none: a first frame with the
	// panel closed would read them before any draw forgets them.
	if (assets->arena == NULL) {
		assets->arena = voe_base_arena_new(ASSETS_ARENA);
		assets->up_button = VOE_UI_NODE_NONE;
		assets->import_button = VOE_UI_NODE_NONE;
		assets->naming_field = VOE_UI_NODE_NONE;
		assets->empty = VOE_UI_NODE_NONE;
		assets->body = VOE_UI_NODE_NONE;
	}
	if (!assets->listed_once ||
	    !same_folder(project_folder, assets->project)) {
		relist(assets, project_folder, "", NULL, false);
		assets->listed = now;
	} else if (now - assets->listed >= LIST_SECONDS) {
		relist(assets, assets->project, assets->shown, NULL, false);
		assets->listed = now;
	}
}

void voe_editor_assets_list_due(voe_editor_assets *assets)
{
	VOE_BASE_ASSERT(assets != NULL, "marking no Assets panel due");

	// The frame clock is never below 0, so a second has always passed.
	assets->listed = -LIST_SECONDS;
	VOE_BASE_ASSERT(assets->listed < 0.0, "a listing marked due in the future");
}

// The naming's field holding `text`, focused on the frame its naming began.
// A refused field (VOE_UI_NODE_NONE) has nothing to focus, so the focus waits.
static void naming_field_draw(voe_ui_context *ui, voe_editor_assets *assets,
			      const char *text)
{
	VOE_BASE_ASSERT(ui != NULL && assets != NULL, "drawing no naming field");
	VOE_BASE_ASSERT(text != NULL, "a naming field holding no name");
	assets->naming_field =
		voe_ui_field(ui, "asset_name", 0, text, (voe_ui_sizing){ 0 });
	if (assets->naming_focus && assets->naming_field != VOE_UI_NODE_NONE) {
		voe_ui_field_focus(ui, assets->naming_field);
		assets->naming_focus = false;
	}
}

void voe_editor_assets_draw(voe_ui_context *ui, voe_editor_assets *assets)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing the Assets panel into no interface");
	VOE_BASE_ASSERT(assets != NULL, "drawing no Assets panel");

	assets->up_button = VOE_UI_NODE_NONE;
	assets->import_button = VOE_UI_NODE_NONE;
	assets->naming_field = VOE_UI_NODE_NONE;
	assets->empty = VOE_UI_NODE_NONE;
	// Round everything, spaced as the leaf's scroll area spaces it (dock.c),
	// so the read knows the whole panel's rectangle.
	assets->body = voe_ui_column_begin(
		ui, (voe_ui_container){
			    .size = { .along = { VOE_UI_SIZE_GROW, 1.0f } },
			    .across = VOE_UI_ACROSS_FILL,
			    .gap = ASSETS_GAP });
	// Import needs a folder to copy into, so an untitled project has none;
	// a missing `Assets/` still has it, since importing makes one.
	if (assets->listed_once && assets->project != NULL) {
		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ASSETS_GAP });
		if (!assets->missing && assets->shown[0] != '\0') {
			assets->up_button =
				voe_ui_button_begin(ui, "assets_up", 0);
			voe_ui_label(ui, "Up");
			voe_ui_end(ui);
		}
		assets->import_button = voe_ui_button_begin(ui, "assets_import", 0);
		voe_ui_label(ui, "Import");
		voe_ui_end(ui);
		voe_ui_end(ui); // button row
	}
	if (!assets->listed_once || assets->missing) {
		voe_ui_label(ui, MISSING_LINE);
		voe_ui_end(ui); // body
		return;
	}
	voe_ui_label(ui, assets->path);
	// Folders come first, so the pending folder is first among them.
	if (assets->naming == VOE_EDITOR_ASSETS_NAMING_FOLDER)
		naming_field_draw(ui, assets, NEW_FOLDER_NAME);
	for (uint32_t i = 0; i < assets->row_count; i++) {
		voe_editor_assets_row *row = &assets->rows[i];
		bool selected = row->name == assets->selected;

		// The pending landscape before the first file.
		if (!row->folder && (i == 0 || assets->rows[i - 1].folder) &&
		    assets->naming == VOE_EDITOR_ASSETS_NAMING_LANDSCAPE)
			naming_field_draw(ui, assets, NEW_LANDSCAPE_NAME);

		if (selected &&
		    assets->naming == VOE_EDITOR_ASSETS_NAMING_RENAME) {
			row->node = VOE_UI_NODE_NONE;
			naming_field_draw(ui, assets, row->name);
			continue;
		}
		row->node = voe_ui_choice_begin(ui, "asset", i, selected);
		voe_ui_label(ui, row->name);
		if (row->folder)
			voe_ui_label(ui, "/");
		voe_ui_end(ui);
	}
	// No file row to go before: after the folders.
	if (assets->naming == VOE_EDITOR_ASSETS_NAMING_LANDSCAPE &&
	    (assets->row_count == 0 ||
	     assets->rows[assets->row_count - 1].folder))
		naming_field_draw(ui, assets, NEW_LANDSCAPE_NAME);
	assets->empty = voe_ui_column_begin(
		ui, (voe_ui_container){
			    .size = { .along = { VOE_UI_SIZE_GROW, 1.0f } } });
	voe_ui_end(ui);
	voe_ui_end(ui); // body
}

// `folder` and `name` joined with `/` into `out`, or `name` alone at
// `Assets/`; voe_editor_assets_rename_begin left the room.
static void joined_into(char *out, const char *folder, const char *name)
{
	int length;

	VOE_BASE_ASSERT(out != NULL && folder != NULL && name != NULL,
			"joining no path");
	length = folder[0] == '\0' ?
			 snprintf(out, VOE_EDITOR_ASSETS_PATH, "%s", name) :
			 snprintf(out, VOE_EDITOR_ASSETS_PATH, "%s/%s", folder,
				  name);
	VOE_BASE_ASSERT(length >= 0 && length < VOE_EDITOR_ASSETS_PATH,
			"a request's path past the room naming checked for");
}

// The committed `name` as the naming's request, the shown folder's `\` from
// a joined subfolder made `/`, as assets_manage.h's paths are.
static void request_set(voe_editor_assets *assets, const char *name)
{
	voe_editor_assets_request *request = &assets->request;

	VOE_BASE_ASSERT(assets->shown != NULL && name != NULL,
			"a request from no listed panel or no name");
	VOE_BASE_ASSERT(assets->naming != VOE_EDITOR_ASSETS_NAMING_RENAME ||
				assets->selected != NULL,
			"a rename with no selected row");
	*request = (voe_editor_assets_request){ .kind = assets->naming };
	snprintf(request->name, sizeof request->name, "%s", name);
	joined_into(request->folder, "", assets->shown);
	for (char *c = request->folder; *c != '\0'; c++)
		if (*c == '\\')
			*c = '/';
	if (request->kind != VOE_EDITOR_ASSETS_NAMING_RENAME)
		return;
	joined_into(request->from, request->folder, assets->selected);
	joined_into(request->to, request->folder, request->name);
}

// The naming's field read: a commit leaves its request, and a commit, a
// cancel, a focus lost or a field not drawn this frame ends the naming.
static void naming_read(const voe_ui_context *ui, voe_editor_assets *assets)
{
	voe_ui_field_result result;

	VOE_BASE_ASSERT(ui != NULL && assets != NULL, "reading no naming");
	if (assets->naming == VOE_EDITOR_ASSETS_NAMING_NONE)
		return;
	if (assets->naming_field == VOE_UI_NODE_NONE) {
		assets->naming = VOE_EDITOR_ASSETS_NAMING_NONE;
		return;
	}
	result = voe_ui_field_action(ui, assets->naming_field);
	assets->naming_field = VOE_UI_NODE_NONE;
	if (result.committed)
		request_set(assets, result.text);
	if (result.committed || result.cancelled || !result.focused)
		assets->naming = VOE_EDITOR_ASSETS_NAMING_NONE;
	VOE_BASE_ASSERT(assets->naming_field == VOE_UI_NODE_NONE,
			"a naming field left standing after its read");
}

// `name` in the shown folder as `Assets/...` into `opened`, `\` from a joined
// subfolder made `/`; left "" when it does not fit, as assets_drag.c refuses.
static void opened_set(voe_editor_assets *assets, const char *name)
{
	int length;

	VOE_BASE_ASSERT(assets != NULL && assets->shown != NULL,
			"opening a prefab from no listed panel");
	VOE_BASE_ASSERT(name != NULL, "opening a prefab with no name");
	length = assets->shown[0] == '\0' ?
			 snprintf(assets->opened, sizeof assets->opened,
				  "Assets/%s", name) :
			 snprintf(assets->opened, sizeof assets->opened,
				  "Assets/%s/%s", assets->shown, name);
	if (length < 0 || (size_t)length >= sizeof assets->opened) {
		assets->opened[0] = '\0';
		return;
	}
	for (char *c = assets->opened; *c != '\0'; c++)
		if (*c == '\\')
			*c = '/';
}

// Where `node` showed this frame, no size for one not drawn.
static voe_ui_rect seen(const voe_ui_context *ui, voe_ui_node node)
{
	VOE_BASE_ASSERT(ui != NULL, "a rectangle from no interface");
	if (node == VOE_UI_NODE_NONE)
		return (voe_ui_rect){ 0 };
	return voe_ui_node_visible(ui, node);
}

bool voe_editor_assets_clicks_read(const voe_ui_context *ui,
				   voe_editor_assets *assets, bool pointer_down,
				   bool over)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(assets != NULL, "reading the clicks of no Assets panel");
	VOE_BASE_ASSERT(assets->row_count <= VOE_EDITOR_BROWSER_ROWS,
			"more Assets rows than the panel holds");

	bool up = assets->up_button != VOE_UI_NODE_NONE &&
		  voe_ui_button_action(ui, assets->up_button).fired;
	bool import = assets->import_button != VOE_UI_NODE_NONE &&
		      voe_ui_button_action(ui, assets->import_button).fired;
	const char *entered = NULL;
	bool pressed = false;
	// A dragged row's release over itself, or over a row the move's listing
	// put at its index, is not a click.
	const bool dragged = assets->dragged;

	// Before the rows: a press on another row commits the rename of the
	// one still selected.
	naming_read(ui, assets);
	// Every node is forgotten once read, so a frame that does not draw
	// the panel asks nothing stale; where it showed is kept for the right
	// button, no size for one not drawn.
	assets->body_seen = seen(ui, assets->body);
	assets->empty_seen = seen(ui, assets->empty);
	assets->up_seen = seen(ui, assets->up_button);
	assets->up_button = VOE_UI_NODE_NONE;
	assets->import_button = VOE_UI_NODE_NONE;
	assets->empty = VOE_UI_NODE_NONE;
	assets->body = VOE_UI_NODE_NONE;
	assets->held = NULL;
	assets->held_model = false;
	assets->held_prefab = false;
	assets->held_picture = false;
	assets->dragged = false;
	for (uint32_t i = 0; i < assets->row_count; i++) {
		voe_editor_assets_row *row = &assets->rows[i];
		bool held = row->node != VOE_UI_NODE_NONE &&
			    voe_ui_button_action(ui, row->node).held;

		if (held) {
			assets->selected = row->name;
			pressed = true;
			assets->held = row->name;
			assets->held_model = row->model;
			assets->held_prefab = row->prefab;
			assets->held_picture = row->picture;
		}
		if (entered == NULL && row->folder && !dragged &&
		    row->node != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, row->node).fired)
			entered = row->name;
		if (row->prefab && !dragged && row->node != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, row->node).fired)
			opened_set(assets, row->name);
		row->seen = seen(ui, row->node);
		row->node = VOE_UI_NODE_NONE;
	}
	// A row's own press takes the keyboard; one outside the panel, not a
	// row's drag carried out of it, gives it back.
	if (pressed)
		assets->keyboard = true;
	else if (pointer_down && !over)
		assets->keyboard = false;
	if (up || entered != NULL) {
		relist(assets, assets->project, assets->shown, entered, up);
		assets->selected = NULL;
	}
	return import;
}

// Whether the shown folder leaves room in a request for a whole field's name.
static bool naming_fits(const voe_editor_assets *assets)
{
	VOE_BASE_ASSERT(assets != NULL, "asking whether no panel can name");
	return assets->listed_once && !assets->missing &&
	       strlen(assets->shown) + 1 + VOE_UI_FIELD_CAPACITY <
		       VOE_EDITOR_ASSETS_PATH;
}

void voe_editor_assets_rename_begin(voe_editor_assets *assets)
{
	VOE_BASE_ASSERT(assets != NULL, "renaming in no Assets panel");
	if (assets->selected == NULL || !naming_fits(assets))
		return;
	assets->naming = VOE_EDITOR_ASSETS_NAMING_RENAME;
	assets->naming_focus = true;
	VOE_BASE_ASSERT(assets->selected != NULL, "a rename with no row");
}

void voe_editor_assets_folder_begin(voe_editor_assets *assets)
{
	VOE_BASE_ASSERT(assets != NULL, "naming a folder in no Assets panel");
	if (!naming_fits(assets))
		return;
	assets->naming = VOE_EDITOR_ASSETS_NAMING_FOLDER;
	assets->naming_focus = true;
	VOE_BASE_ASSERT(assets->naming_focus, "a folder naming never focused");
}

void voe_editor_assets_landscape_begin(voe_editor_assets *assets)
{
	VOE_BASE_ASSERT(assets != NULL, "naming a landscape in no Assets panel");
	if (!naming_fits(assets))
		return;
	assets->naming = VOE_EDITOR_ASSETS_NAMING_LANDSCAPE;
	assets->naming_focus = true;
	VOE_BASE_ASSERT(assets->naming_focus, "a landscape naming never focused");
}

bool voe_editor_assets_selected_path(const voe_editor_assets *assets, char *out,
				     size_t size)
{
	int length;

	VOE_BASE_ASSERT(assets != NULL, "a path in no Assets panel");
	VOE_BASE_ASSERT(out != NULL && size > 0, "a path into nowhere");
	out[0] = '\0';
	if (assets->selected == NULL || !assets->listed_once || assets->missing)
		return false;
	length = assets->shown[0] == '\0' ?
			 snprintf(out, size, "%s", assets->selected) :
			 snprintf(out, size, "%s/%s", assets->shown,
				  assets->selected);
	if (length < 0 || (size_t)length >= size) {
		out[0] = '\0';
		return false;
	}
	for (char *c = out; *c != '\0'; c++)
		if (*c == '\\')
			*c = '/';
	return true;
}

void voe_editor_assets_delete_begin(voe_editor_assets *assets)
{
	VOE_BASE_ASSERT(assets != NULL, "deleting in no Assets panel");
	if (!voe_editor_assets_selected_path(assets, assets->deleting,
					     sizeof assets->deleting))
		return;
	VOE_BASE_ASSERT(assets->deleting[0] != '\0', "a delete asked about nothing");
}

void voe_editor_assets_held_dragged(voe_editor_assets *assets)
{
	VOE_BASE_ASSERT(assets != NULL, "dragging in no Assets panel");
	assets->dragged = true;
	VOE_BASE_ASSERT(assets->dragged, "a drag not marked");
}

voe_editor_assets_at voe_editor_assets_row_at(const voe_editor_assets *assets,
					      voe_math_float2 at)
{
	VOE_BASE_ASSERT(assets != NULL, "asking what is under a point in no panel");
	VOE_BASE_ASSERT(assets->row_count <= VOE_EDITOR_BROWSER_ROWS,
			"more Assets rows than the panel holds");
	if (!voe_editor_inspector_rect_contains(assets->body_seen, at))
		return (voe_editor_assets_at){ VOE_EDITOR_ASSETS_AT_OUTSIDE, 0 };
	if (voe_editor_inspector_rect_contains(assets->up_seen, at))
		return (voe_editor_assets_at){ VOE_EDITOR_ASSETS_AT_UP, 0 };
	for (uint32_t i = 0; i < assets->row_count; i++)
		if (voe_editor_inspector_rect_contains(assets->rows[i].seen, at))
			return (voe_editor_assets_at){ VOE_EDITOR_ASSETS_AT_ROW, i };
	if (voe_editor_inspector_rect_contains(assets->empty_seen, at))
		return (voe_editor_assets_at){ VOE_EDITOR_ASSETS_AT_EMPTY, 0 };
	return (voe_editor_assets_at){ VOE_EDITOR_ASSETS_AT_NONE, 0 };
}

bool voe_editor_assets_menu_at(voe_editor_assets *assets, voe_math_float2 at)
{
	voe_editor_assets_at under;

	VOE_BASE_ASSERT(assets != NULL, "a right press on no Assets panel");
	under = voe_editor_assets_row_at(assets, at);
	if (under.kind == VOE_EDITOR_ASSETS_AT_ROW)
		assets->selected = assets->rows[under.row].name;
	if (under.kind != VOE_EDITOR_ASSETS_AT_ROW &&
	    under.kind != VOE_EDITOR_ASSETS_AT_EMPTY)
		return false;
	assets->keyboard = true;
	voe_editor_assets_menu_open(&assets->menu, at,
				    under.kind == VOE_EDITOR_ASSETS_AT_ROW);
	VOE_BASE_ASSERT(assets->menu.open, "a right press that opened no menu");
	return true;
}

// Makes `<project>/Assets/` when the project's listing has none. False with
// why naming the folder when the listing or the make fails.
static bool assets_made(const char *project, voe_base_arena *scratch,
			voe_editor_notice *why)
{
	voe_platform_folder_listing listing;
	const char *folder;
	bool missing;

	VOE_BASE_ASSERT(project != NULL && scratch != NULL, "making no Assets/");
	VOE_BASE_ASSERT(why != NULL, "making Assets/ with nowhere to say why");
	if (!list_shown(project, "", scratch, &listing, &missing)) {
		voe_editor_notice_from_report(why, project);
		return false;
	}
	if (!missing)
		return true;
	folder = voe_platform_path_join(scratch, project, ASSETS_FOLDER);
	voe_base_report_error_clear();
	if (!voe_platform_folder_create(folder, NULL)) {
		voe_editor_notice_from_report(why, folder);
		return false;
	}
	return true;
}

void voe_editor_assets_import(voe_editor_assets *assets,
			      const char *project_folder, const char *source,
			      voe_editor_notice *why)
{
	voe_base_arena *scratch;
	const char *shown;
	const char *target;
	const uint8_t *bytes;
	size_t count;

	VOE_BASE_ASSERT(assets != NULL && assets->arena != NULL,
			"importing into an Assets panel never listed");
	VOE_BASE_ASSERT(project_folder != NULL && source != NULL,
			"importing with no project folder or no file");
	VOE_BASE_ASSERT(why != NULL, "importing with nowhere to say why");

	// The shown folder is only this project's when it was listed for it.
	shown = same_folder(project_folder, assets->project) &&
				!assets->missing ?
			assets->shown :
			"";
	// Read before anything is made, so a source that fails leaves the
	// project as it was.
	scratch = voe_base_arena_new(ASSETS_SCRATCH);
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(source, scratch, &count, NULL);
	if (bytes == NULL) {
		voe_editor_notice_from_report(why, source);
		goto destroy_scratch;
	}
	// A write refuses no bytes (platform/file.h), so an empty file is
	// refused here, naming it, rather than asserting there.
	if (count == 0) {
		voe_editor_notice_set(why, "%s: is empty", source);
		goto destroy_scratch;
	}
	if (!assets_made(project_folder, scratch, why))
		goto destroy_scratch;
	target = voe_platform_path_join(scratch, project_folder, ASSETS_FOLDER);
	if (shown[0] != '\0')
		target = voe_platform_path_join(scratch, target, shown);
	target = voe_platform_path_join(scratch, target,
					voe_platform_path_name(source));

	voe_base_report_error_clear();
	if (!voe_platform_file_write(target, bytes, count, NULL)) {
		voe_editor_notice_from_report(why, target);
		goto destroy_scratch;
	}
	relist(assets, project_folder, shown, NULL, false);
destroy_scratch:
	voe_base_arena_destroy(scratch);
}

void voe_editor_assets_destroy(voe_editor_assets *assets)
{
	VOE_BASE_ASSERT(assets != NULL, "destroying no Assets panel");

	if (assets->arena != NULL)
		voe_base_arena_destroy(assets->arena);
	*assets = (voe_editor_assets){ 0 };
}
