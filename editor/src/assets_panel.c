// The Assets panel's listing, its one frame of `ui` calls and the read of its
// rows and Up afterwards. See the header for the arena, the once a second and
// what a missing `Assets/` or a failed listing leaves.
#include "assets_panel.h"

#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <ctype.h>
#include <string.h>

// Block sizes, not limits, as browser.c's are.
#define ASSETS_ARENA (16u * 1024u)
#define ASSETS_SCRATCH (64u * 1024u)
#define ASSETS_FOLDER "Assets"
#define LIST_SECONDS 1.0
#define MISSING_LINE "No Assets folder in this project."
#define ASSETS_GAP 2.0f

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

// Whether `name` ends `.prefab`, in any case.
static bool names_prefab(const char *name)
{
	static const char prefab[] = ".prefab";
	size_t length;

	VOE_BASE_ASSERT(name != NULL, "asking whether no name is a prefab");
	length = strlen(name);
	if (length < sizeof prefab - 1)
		return false;
	name += length - (sizeof prefab - 1);
	for (size_t i = 0; i < sizeof prefab - 1; i++)
		if (tolower((unsigned char)name[i]) != prefab[i])
			return false;
	return true;
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
				.model = !e->folder &&
					 voe_editor_browser_names_model(e->name),
				.prefab = !e->folder && names_prefab(e->name),
			};
		}
	}
}

// Lists `project`'s `shown`, entered into `name` when it is not NULL or up a
// level when `up`, and commits it to the arena. Both strings are copied into
// scratch first: either may point into the arena being cleared.
static void relist(voe_editor_assets *assets, const char *project,
		   const char *shown, const char *name, bool up)
{
	voe_base_arena *scratch;
	voe_platform_folder_listing listing;
	const char *kept;
	char *next;
	bool missing;
	bool same;

	VOE_BASE_ASSERT(assets != NULL && assets->arena != NULL, "listing no panel");
	VOE_BASE_ASSERT(shown != NULL, "listing no shown folder");
	scratch = voe_base_arena_new(ASSETS_SCRATCH);
	kept = project ? copy_string(scratch, project) : NULL;
	next = copy_string(scratch, shown);
	same = assets->listed_once && same_folder(kept, assets->project);

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
	voe_base_arena_destroy(scratch);
}

void voe_editor_assets_update(voe_editor_assets *assets,
			      const char *project_folder, double now)
{
	VOE_BASE_ASSERT(assets != NULL, "updating no Assets panel");
	VOE_BASE_ASSERT(now >= 0.0, "a frame clock before its own start");

	if (assets->arena == NULL)
		assets->arena = voe_base_arena_new(ASSETS_ARENA);
	if (!assets->listed_once ||
	    !same_folder(project_folder, assets->project)) {
		relist(assets, project_folder, "", NULL, false);
		assets->listed = now;
	} else if (now - assets->listed >= LIST_SECONDS) {
		relist(assets, assets->project, assets->shown, NULL, false);
		assets->listed = now;
	}
}

void voe_editor_assets_draw(voe_ui_context *ui, voe_editor_assets *assets)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing the Assets panel into no interface");
	VOE_BASE_ASSERT(assets != NULL, "drawing no Assets panel");

	assets->up_button = VOE_UI_NODE_NONE;
	assets->import_button = VOE_UI_NODE_NONE;
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
		return;
	}
	voe_ui_label(ui, assets->path);
	for (uint32_t i = 0; i < assets->row_count; i++) {
		voe_editor_assets_row *row = &assets->rows[i];

		// A model or prefab row is marked the way the Scene list marks
		// its selected row: a choice drawn inverted (ADR-0194).
		row->node = voe_ui_choice_begin(ui, "asset", i,
						row->model || row->prefab);
		voe_ui_label(ui, row->name);
		if (row->folder)
			voe_ui_label(ui, "/");
		voe_ui_end(ui);
	}
}

bool voe_editor_assets_clicks_read(const voe_ui_context *ui,
				   voe_editor_assets *assets)
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

	// Every node is forgotten once read, so a frame that does not draw
	// the panel asks nothing stale.
	assets->up_button = VOE_UI_NODE_NONE;
	assets->import_button = VOE_UI_NODE_NONE;
	assets->held = NULL;
	assets->held_prefab = false;
	for (uint32_t i = 0; i < assets->row_count; i++) {
		voe_editor_assets_row *row = &assets->rows[i];

		if ((row->model || row->prefab) &&
		    row->node != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, row->node).held) {
			assets->held = row->name;
			assets->held_prefab = row->prefab;
		}
		if (entered == NULL && row->folder &&
		    row->node != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, row->node).fired)
			entered = row->name;
		row->node = VOE_UI_NODE_NONE;
	}
	if (up || entered != NULL)
		relist(assets, assets->project, assets->shown, entered, up);
	return import;
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
