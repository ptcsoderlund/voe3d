// The browser's listing, its one frame of `ui` calls, and the read of its
// buttons and rows afterwards. See the header for who owns the arena and why a
// failed listing changes nothing.
#include "browser.h"

#include <authoring/project.h>

#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <ui/widgets.h>

#include <ctype.h>
#include <string.h>

// The browser's own long-lived room for its current folder's path and rows,
// and the scratch every navigation does its listing in. Block sizes, not
// limits: a folder with many long names gets another block, same as any
// arena here.
#define VOE_EDITOR_BROWSER_ARENA (64u * 1024u)
#define VOE_EDITOR_BROWSER_SCRATCH (256u * 1024u)

// The plate behind the browser is the theme's RAISED surface — a floating
// panel covering another, exactly what that role is for (ui/theme.h).
#define BROWSER_PAD 3.0f
#define BROWSER_GAP 2.0f

// What a marked row's label ends in.
#define PROJECT_MARK " — project"
#define FILE_MARK " — file"

// SAVE mode's own button, beside the name field. Literal, for the same
// reason PROJECT_MARK is.
#define MAKE_FOLDER_TEXT "Make folder"

// A copy of text, NUL included, in arena — the row names and the folder path
// all outlive the scratch arena they are first read into, because they are
// this call's own way of committing to arena rather than a pointer into
// somebody else's.
static const char *copy_string(voe_base_arena *arena, const char *text)
{
	size_t size = strlen(text) + 1;
	char *copy = voe_base_arena_push(arena, size);

	memcpy(copy, text, size);
	return copy;
}

// Whether folder/name/project.voe3d exists, using scratch for the two joins —
// neither has to outlive this call.
static bool marks_project(voe_base_arena *scratch, const char *folder,
			  const char *name)
{
	const char *entry = voe_platform_path_join(scratch, folder, name);
	const char *project_path =
		voe_platform_path_join(scratch, entry, VOE_AUTHORING_PROJECT_FILE);

	return voe_platform_file_exists(project_path);
}

bool voe_editor_browser_names_model(const char *name)
{
	static const char glb[] = ".glb";
	size_t length;

	VOE_BASE_ASSERT(name != NULL, "asking whether no name is a model");
	length = strlen(name);

	if (length < sizeof(glb) - 1)
		return false;
	name += length - (sizeof(glb) - 1);
	for (size_t i = 0; i < sizeof(glb) - 1; i++)
		if (tolower((unsigned char)name[i]) != glb[i])
			return false;
	return true;
}

// Whether entry is a row in mode: a visible folder always, a visible `.glb`
// file in IMPORT mode only.
static bool lists(const voe_platform_folder_entry *entry,
		  voe_editor_browser_mode mode)
{
	VOE_BASE_ASSERT(entry != NULL, "asking whether no entry is a row");
	if (entry->hidden)
		return false;
	return entry->folder || (mode == VOE_EDITOR_BROWSER_IMPORT &&
				 voe_editor_browser_names_model(entry->name));
}

// Lists candidate into scratch and, only once that has succeeded, clears
// browser->arena and rebuilds browser->folder and browser->rows from it —
// see the header on why the order is this way round. candidate may be
// scratch's own: nothing here keeps a pointer into it past this call, because
// everything kept is copied into browser->arena first. Folders in the first
// pass, IMPORT's files in the second.
static bool relist(voe_editor_browser *browser, const char *candidate,
		   voe_base_arena *scratch, voe_editor_notice *why)
{
	voe_platform_folder_listing listing;
	voe_base_error error;
	const char *folder_copy;

	voe_base_report_error_clear();
	if (!voe_platform_folder_list(candidate, scratch, &listing, &error)) {
		voe_editor_notice_from_report(why, candidate);
		return false;
	}

	voe_base_arena_clear(browser->arena);
	folder_copy = copy_string(browser->arena, candidate);

	browser->row_count = 0;
	for (uint32_t pass = 0; pass < 2; pass++) {
		for (uint32_t i = 0; i < listing.count &&
				     browser->row_count < VOE_EDITOR_BROWSER_ROWS;
		     i++) {
			const voe_platform_folder_entry *entry =
				&listing.entries[i];

			if (entry->folder != (pass == 0) ||
			    !lists(entry, browser->mode))
				continue;

			browser->rows[browser->row_count++] =
				(voe_editor_browser_row){
					.node = VOE_UI_NODE_NONE,
					.name = copy_string(browser->arena,
							    entry->name),
					.project = entry->folder &&
						   marks_project(scratch,
								 candidate,
								 entry->name),
					.file = !entry->folder,
				};
		}
	}

	browser->folder = folder_copy;
	// EVERY NAVIGATION LANDS HERE, so this is where a choice is cleared.
	browser->chosen = VOE_EDITOR_BROWSER_NO_ROW;
	browser->target = browser->folder;
	return true;
}

// Lists beside's parent and chooses beside's row in it. False, with nothing
// said, when there is no parent, it will not list or it holds no such row:
// the caller then starts as a first showing does. beside is scratch's own
// copy, so the relist clearing browser->arena cannot take it away.
static bool choose_beside(voe_editor_browser *browser, const char *beside,
			  voe_base_arena *scratch)
{
	const char *parent = voe_platform_path_parent(scratch, beside);
	const char *name = voe_platform_path_name(beside);
	voe_editor_notice unsaid = { 0 };

	VOE_BASE_ASSERT(beside != NULL, "choosing beside no folder");
	if (parent == NULL || !relist(browser, parent, scratch, &unsaid))
		return false;

	for (uint32_t i = 0; i < browser->row_count; i++) {
		if (browser->rows[i].file ||
		    strcmp(browser->rows[i].name, name) != 0)
			continue;
		browser->chosen = i;
		if (browser->mode == VOE_EDITOR_BROWSER_OPEN)
			browser->target = voe_platform_path_join(
				browser->arena, browser->folder, name);
		VOE_BASE_ASSERT(browser->target != NULL,
				"a chosen row with no folder to act on");
		return true;
	}
	return false;
}

void voe_editor_browser_show(voe_editor_browser *browser,
			     voe_editor_browser_mode mode, const char *beside,
			     voe_editor_notice *why)
{
	VOE_BASE_ASSERT(browser != NULL, "showing no browser");
	VOE_BASE_ASSERT(why != NULL, "showing a browser with nowhere to say why");

	if (browser->arena == NULL) {
		browser->arena = voe_base_arena_new(VOE_EDITOR_BROWSER_ARENA);
		browser->chosen = VOE_EDITOR_BROWSER_NO_ROW;
	}

	browser->mode = mode;
	browser->showing = true;
	// A SAVE SHOWING TAKES THE KEYBOARD TO THE NAME BOX ON THE FRAME IT
	// DRAWS FIRST — the next voe_editor_browser_draw consumes this.
	// `name` itself is left as it was: it keeps whatever a previous SAVE
	// showing typed, the same "for the session" the folder already gets.
	browser->focus_name = mode == VOE_EDITOR_BROWSER_SAVE;

	// A SHOWING BESIDE A FOLDER STARTS IN ITS PARENT, or as a first one
	// does when it cannot; THE FIRST SHOWING EVER PICKS A FOLDER; EVERY
	// OTHER ONE KEEPS WHAT IT HAD (the header's "across showings, for the
	// session") and lists it again in this mode.
	voe_base_arena *scratch = voe_base_arena_new(VOE_EDITOR_BROWSER_SCRATCH);
	voe_base_error error;
	const char *start = browser->folder;

	if (beside != NULL) {
		if (choose_beside(browser, copy_string(scratch, beside),
				  scratch)) {
			voe_base_arena_destroy(scratch);
			return;
		}
		start = NULL;
	}
	if (start == NULL)
		start = voe_platform_folder_home(scratch);
	if (start == NULL)
		start = voe_platform_path_absolute(".", scratch, &error);

	if (start != NULL)
		relist(browser, copy_string(scratch, start), scratch, why);
	else
		voe_editor_notice_set(why, "no folder to start the browser in");
	voe_base_arena_destroy(scratch);
}

void voe_editor_browser_hide(voe_editor_browser *browser)
{
	VOE_BASE_ASSERT(browser != NULL, "hiding no browser");

	browser->showing = false;
}

void voe_editor_browser_enter(voe_editor_browser *browser, const char *name,
			      voe_editor_notice *why)
{
	voe_base_arena *scratch;
	const char *candidate;

	VOE_BASE_ASSERT(browser != NULL, "entering a row of no browser");
	VOE_BASE_ASSERT(browser->folder != NULL,
			"entering a row before the browser has a folder");
	VOE_BASE_ASSERT(name != NULL, "entering no row's name");
	VOE_BASE_ASSERT(why != NULL, "entering a row with nowhere to say why");

	scratch = voe_base_arena_new(VOE_EDITOR_BROWSER_SCRATCH);
	candidate = voe_platform_path_join(scratch, browser->folder, name);
	relist(browser, candidate, scratch, why);
	voe_base_arena_destroy(scratch);
}

void voe_editor_browser_up(voe_editor_browser *browser, voe_editor_notice *why)
{
	voe_base_arena *scratch;
	const char *parent;

	VOE_BASE_ASSERT(browser != NULL, "going up in no browser");
	VOE_BASE_ASSERT(browser->folder != NULL,
			"going up before the browser has a folder");
	VOE_BASE_ASSERT(why != NULL, "going up with nowhere to say why");

	scratch = voe_base_arena_new(VOE_EDITOR_BROWSER_SCRATCH);
	// NULL at a root: platform/path.h's own answer, and this file's own
	// "does nothing at a root".
	parent = voe_platform_path_parent(scratch, browser->folder);
	if (parent != NULL)
		relist(browser, parent, scratch, why);
	voe_base_arena_destroy(scratch);
}

void voe_editor_browser_draw(voe_ui_context *ui, voe_editor_browser *browser,
			     float top, voe_math_float2 size)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing no browser into no interface");
	VOE_BASE_ASSERT(browser != NULL, "drawing no browser");
	VOE_BASE_ASSERT(browser->showing, "drawing a browser that is not showing");

	// ANCHORED FILL ON X, A FIXED HEIGHT STARTING top DOWN ON Y — the
	// area below the bar, read absolutely because an anchored child's
	// size is (layout.h): x is width, y is height, whatever this
	// container's own flow would otherwise have meant them as.
	voe_ui_panel_begin(
		ui, "browser", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.across = VOE_UI_ACROSS_FILL,
			.gap = BROWSER_GAP,
			.pad = { BROWSER_PAD, BROWSER_PAD, BROWSER_PAD,
				 BROWSER_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_FILL, 0.0f },
				    .y = { VOE_UI_ACROSS_START, top } },
			.size = { .across = { VOE_UI_SIZE_FIXED, size.y } } });

	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = BROWSER_GAP });
	voe_ui_label(ui, browser->folder != NULL ? browser->folder : "");
	browser->up_button = voe_ui_button_begin(ui, "up", 0);
	voe_ui_label(ui, "Up");
	voe_ui_end(ui); // up button
	voe_ui_end(ui); // top row

	voe_ui_scroll_begin(
		ui, "browser_rows", 0,
		(voe_ui_container){ .size = { .along = { VOE_UI_SIZE_GROW,
							 1.0f } },
				     .across = VOE_UI_ACROSS_FILL,
				     .gap = BROWSER_GAP },
		(voe_ui_scroll_axes){ .y = true });
	for (uint32_t i = 0; i < browser->row_count; i++) {
		browser->rows[i].node = voe_ui_choice_begin(
			ui, "row", i, i == browser->chosen);
		voe_ui_label(ui, browser->rows[i].name);
		if (browser->rows[i].project)
			voe_ui_label(ui, PROJECT_MARK);
		if (browser->rows[i].file)
			voe_ui_label(ui, FILE_MARK);
		voe_ui_end(ui);
	}
	voe_ui_end(ui); // scroll area

	// THE NAME ROW, SAVE MODE ONLY. Outside it neither node is ever drawn,
	// so both are set fresh to VOE_UI_NODE_NONE here rather than carrying
	// over whatever a previous SAVE showing left in them — the same
	// freshness the three fixed buttons above already get every call.
	if (browser->mode == VOE_EDITOR_BROWSER_SAVE) {
		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = BROWSER_GAP });
		browser->name_field = voe_ui_field(
			ui, "name", 0, browser->name,
			(voe_ui_sizing){ .along = { VOE_UI_SIZE_GROW,
						    1.0f } });
		// CONSUMED HERE, ONCE — the frame voe_editor_browser_show
		// armed it for, and never again until the next SAVE showing.
		// A refused field (VOE_UI_NODE_NONE) has nothing to focus —
		// asking would assert (ui/widgets.h) — so the flag waits for
		// a frame that actually laid one out.
		if (browser->focus_name &&
		    browser->name_field != VOE_UI_NODE_NONE) {
			voe_ui_field_focus(ui, browser->name_field);
			browser->focus_name = false;
		}
		browser->make_button = voe_ui_button_begin(ui, "make", 0);
		voe_ui_label(ui, MAKE_FOLDER_TEXT);
		voe_ui_end(ui); // make button
		voe_ui_end(ui); // name row
	} else {
		browser->name_field = VOE_UI_NODE_NONE;
		browser->make_button = VOE_UI_NODE_NONE;
	}

	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = BROWSER_GAP });
	// IMPORT HAS NO CONFIRM: pressing a file row is the choice.
	browser->confirm_button = VOE_UI_NODE_NONE;
	if (browser->mode != VOE_EDITOR_BROWSER_IMPORT) {
		browser->confirm_button = voe_ui_button_begin(ui, "confirm", 0);
		voe_ui_label(ui, browser->mode == VOE_EDITOR_BROWSER_OPEN ?
					 "Open" :
					 "Save here");
		voe_ui_end(ui); // confirm button
	}
	browser->cancel_button = voe_ui_button_begin(ui, "cancel", 0);
	voe_ui_label(ui, "Cancel");
	voe_ui_end(ui); // cancel button
	voe_ui_end(ui); // bottom row

	voe_ui_end(ui); // panel
}

voe_editor_browser_result
voe_editor_browser_clicks_read(const voe_ui_context *ui,
			       voe_editor_browser *browser, bool escape)
{
	voe_editor_browser_result result = { .action = VOE_EDITOR_BROWSER_NONE,
					     .name = NULL };
	bool entered = false;

	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(browser != NULL, "reading the clicks of no browser");

	// THE NAME FIELD IS READ FIRST, WHATEVER ELSE THIS FRAME DID — see
	// the header on why every frame writes browser->name back, changed or
	// not. A refused field (VOE_UI_NODE_NONE) has nothing to read, the
	// same skip every other widget below gets.
	if (browser->mode == VOE_EDITOR_BROWSER_SAVE &&
	    browser->name_field != VOE_UI_NODE_NONE) {
		voe_ui_field_result r = voe_ui_field_action(ui,
							    browser->name_field);
		size_t size = strlen(r.text);

		VOE_BASE_ASSERT(
			size <= VOE_UI_FIELD_CAPACITY,
			"a field's own text wider than its own declared capacity");
		memcpy(browser->name, r.text, size + 1);
		entered = r.entered;
	}

	if (escape) {
		result.action = VOE_EDITOR_BROWSER_CANCEL;
		return result;
	}

	// A refused frame hands back VOE_UI_NODE_NONE for every widget past
	// the node budget, and asking one of those what the pointer did is
	// the caller's bug — so they are skipped, exactly as the Scene
	// panel's and the top bar's own recorded nodes are.
	if (browser->up_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, browser->up_button).fired) {
		result.action = VOE_EDITOR_BROWSER_UP;
		return result;
	}

	for (uint32_t i = 0; i < browser->row_count; i++) {
		if (browser->rows[i].node == VOE_UI_NODE_NONE)
			continue;
		if (!voe_ui_button_action(ui, browser->rows[i].node).fired)
			continue;
		if (browser->rows[i].file) {
			result.action = VOE_EDITOR_BROWSER_IMPORT_FILE;
			result.name = voe_platform_path_join(
				browser->arena, browser->folder,
				browser->rows[i].name);
		} else {
			result.action = VOE_EDITOR_BROWSER_ENTERED;
			result.name = browser->rows[i].name;
		}
		return result;
	}

	if (browser->confirm_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, browser->confirm_button).fired) {
		result.action = VOE_EDITOR_BROWSER_CONFIRM;
		return result;
	}

	if (browser->cancel_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, browser->cancel_button).fired) {
		result.action = VOE_EDITOR_BROWSER_CANCEL;
		return result;
	}

	// THE NAME FIELD'S OWN ENTER, OR MAKE FOLDER — either means the same
	// thing, and `name` is browser->name itself: valid as long as nothing
	// else has written it, which is exactly until
	// voe_editor_browser_make_folder is called with it.
	if (browser->mode == VOE_EDITOR_BROWSER_SAVE &&
	    (entered || (browser->make_button != VOE_UI_NODE_NONE &&
			voe_ui_button_action(ui, browser->make_button).fired))) {
		result.action = VOE_EDITOR_BROWSER_MAKE_FOLDER;
		result.name = browser->name;
		return result;
	}

	return result;
}

// Refuses name with a notice in why and returns true for the names
// voe_platform_folder_create must never be asked to make: empty, or
// starting with '.' — which "." and ".." already do — or holding a path
// separator on either platform.
static bool refused_name(voe_editor_notice *why, const char *name)
{
	if (name[0] == '\0' || name[0] == '.') {
		voe_editor_notice_set(why, "\"%s\" is not a folder name", name);
		return true;
	}

	for (const char *c = name; *c != '\0'; c++) {
		if (*c == '/' || *c == '\\') {
			voe_editor_notice_set(why,
					      "\"%s\" is not a folder name",
					      name);
			return true;
		}
	}

	return false;
}

void voe_editor_browser_make_folder(voe_editor_browser *browser,
				    const char *name, voe_editor_notice *why)
{
	char copy[VOE_UI_FIELD_CAPACITY + 1];
	size_t size;
	voe_base_arena *scratch;
	const char *candidate;
	voe_base_error error;

	VOE_BASE_ASSERT(browser != NULL, "making a folder in no browser");
	VOE_BASE_ASSERT(browser->folder != NULL,
			"making a folder before the browser has one to make it in");
	VOE_BASE_ASSERT(name != NULL, "making a folder with no name");
	VOE_BASE_ASSERT(why != NULL, "making a folder with nowhere to say why");

	if (refused_name(why, name))
		return;

	// COPIED BEFORE ANYTHING IS CLEARED. `name` is ordinarily
	// browser->name itself (voe_editor_browser_clicks_read hands it back
	// that way), and browser->name is what gets cleared on success below
	// — so the name voe_editor_browser_enter is asked to enter has to be
	// somewhere else first.
	size = strlen(name);
	VOE_BASE_ASSERT(size <= VOE_UI_FIELD_CAPACITY,
			"a typed name wider than the field that typed it");
	memcpy(copy, name, size + 1);

	scratch = voe_base_arena_new(VOE_EDITOR_BROWSER_SCRATCH);
	candidate = voe_platform_path_join(scratch, browser->folder, copy);

	voe_base_report_error_clear();
	if (!voe_platform_folder_create(candidate, &error)) {
		voe_editor_notice_from_report(why, candidate);
		voe_base_arena_destroy(scratch);
		return;
	}
	voe_base_arena_destroy(scratch);

	browser->name[0] = '\0';
	voe_editor_browser_enter(browser, copy, why);
}

void voe_editor_browser_destroy(voe_editor_browser *browser)
{
	VOE_BASE_ASSERT(browser != NULL, "destroying no browser");

	if (browser->arena != NULL)
		voe_base_arena_destroy(browser->arena);
}
