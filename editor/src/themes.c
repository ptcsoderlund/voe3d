// The themes folder read into a list, and `<settings>/voe3d/theme` read and
// written as its one line. See the header for the contract.
#include "themes.h"

#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <string.h>

// Where the engine's settings live inside <settings>, the folder of theme files
// inside that, the one remembered file name beside it, and what a theme file's
// name ends in.
#define THEMES_SETTINGS "voe3d"
#define THEMES_FOLDER "themes"
#define THEMES_CHOICE "theme"
#define THEMES_SUFFIX ".theme"

// Block sizes, not limits: the list's own (paths, the listing, the entries),
// and one theme's (its file's bytes and the strings read out of them) — also
// the scratch a choose writes its one line from.
#define THEMES_LIST_ARENA (16u * 1024u)
#define THEMES_ONE_ARENA (4u * 1024u)

// path already there, or made one level. False only when neither is true.
// Asked of path's parent's listing rather than of path's own, so a folder
// that is simply not there yet — every first start — is made without a
// failed listing of it being reported on stderr first.
static bool ensure_folder(const char *path, voe_base_arena *scratch)
{
	const char *parent = voe_platform_path_parent(scratch, path);
	const char *name = voe_platform_path_name(path);
	voe_platform_folder_listing listing;

	if (parent != NULL &&
	    voe_platform_folder_list(parent, scratch, &listing, NULL)) {
		for (uint32_t i = 0; i < listing.count; i++)
			if (listing.entries[i].folder &&
			    strcmp(listing.entries[i].name, name) == 0)
				return true;
	} else if (voe_platform_folder_list(path, scratch, &listing, NULL)) {
		return true;
	}
	return voe_platform_folder_create(path, NULL);
}

static bool is_theme_file(const voe_platform_folder_entry *entry)
{
	size_t length = strlen(entry->name);
	size_t suffix = strlen(THEMES_SUFFIX);

	return !entry->folder && length > suffix &&
	       strcmp(entry->name + length - suffix, THEMES_SUFFIX) == 0;
}

static const voe_text_font *font_for(voe_text_typeface typeface,
				     const voe_text_font *oxanium,
				     const voe_text_font *pixel_operator)
{
	return typeface == VOE_TEXT_TYPEFACE_PIXEL_OPERATOR ? pixel_operator :
							      oxanium;
}

// The one line of the remembered file without its newline, pushed into arena,
// or NULL when there is no file or the line is empty.
static const char *read_choice(const char *path, voe_base_arena *arena)
{
	const uint8_t *bytes;
	size_t size;
	char *text;
	char *newline;

	if (!voe_platform_file_exists(path))
		return NULL;
	bytes = voe_platform_file_read(path, arena, &size, NULL);
	if (bytes == NULL)
		return NULL;
	// NUL-terminated past the last byte (platform/file.h).
	text = (char *)bytes;
	newline = memchr(text, '\n', size);
	if (newline != NULL)
		*newline = '\0';
	return text[0] == '\0' ? NULL : text;
}

// Reads one theme file into `entry`, in an arena of its own. False, with that
// arena destroyed and the reason already reported, when it will not read.
static bool read_one(voe_editor_theme *entry, const char *folder,
		     const char *file, voe_base_arena *scratch,
		     const voe_text_font *oxanium,
		     const voe_text_font *pixel_operator)
{
	voe_base_arena *arena = voe_base_arena_new(THEMES_ONE_ARENA);
	const char *path = voe_platform_path_join(scratch, folder, file);
	const uint8_t *bytes;
	size_t size;
	size_t length = strlen(file);
	char *name;

	bytes = voe_platform_file_read(path, arena, &size, NULL);
	if (bytes == NULL ||
	    !voe_theme_read((const char *)bytes, size, arena, &entry->theme)) {
		voe_base_arena_destroy(arena);
		return false;
	}
	name = voe_base_arena_push(arena, length + 1);
	memcpy(name, file, length + 1);

	entry->name = entry->theme.name;
	entry->file = name;
	entry->palette = voe_ui_theme_derive(
		&entry->theme.inputs,
		font_for(entry->theme.typeface, oxanium, pixel_operator));
	entry->arena = arena;
	return true;
}

bool voe_editor_themes_load(voe_editor_themes *themes,
			    const voe_text_font *oxanium,
			    const voe_text_font *pixel_operator)
{
	voe_platform_folder_listing listing = { 0 };
	voe_ui_theme_inputs defaults = voe_ui_theme_default_inputs();
	const char *settings;
	const char *dir = NULL;
	const char *folder = NULL;
	// True once the remembered file has refused, from which point the
	// report is no longer cleared so its refusal stays the first kept one.
	bool remembered_refused = false;

	VOE_BASE_ASSERT(themes != NULL && themes->arena == NULL,
			"loading themes into a list that is not empty");
	VOE_BASE_ASSERT(oxanium != NULL && pixel_operator != NULL,
			"loading themes without both fonts");

	themes->arena = voe_base_arena_new(THEMES_LIST_ARENA);
	themes->remembered = NULL;
	themes->chosen = 0;

	settings = voe_platform_folder_settings(themes->arena);
	if (settings != NULL) {
		dir = voe_platform_path_join(themes->arena, settings,
					     THEMES_SETTINGS);
		folder = voe_platform_path_join(themes->arena, dir,
						THEMES_FOLDER);
		themes->remembered = read_choice(
			voe_platform_path_join(themes->arena, dir,
					       THEMES_CHOICE),
			themes->arena);
		if (!ensure_folder(settings, themes->arena) ||
		    !ensure_folder(dir, themes->arena) ||
		    !ensure_folder(folder, themes->arena) ||
		    !voe_platform_folder_list(folder, themes->arena, &listing,
					      NULL))
			listing = (voe_platform_folder_listing){ 0 };
	}

	themes->entries = voe_base_arena_push(
		themes->arena, sizeof(voe_editor_theme) * (listing.count + 1));
	themes->entries[0] = (voe_editor_theme){
		.name = "Built-in",
		.theme = { .name = "Built-in",
			   .inputs = defaults,
			   .typeface = VOE_TEXT_TYPEFACE_OXANIUM },
		.palette = voe_ui_theme_derive(&defaults, oxanium),
	};
	themes->count = 1;

	for (uint32_t i = 0; i < listing.count; i++) {
		const voe_platform_folder_entry *entry = &listing.entries[i];
		bool remembered = themes->remembered != NULL &&
				  strcmp(entry->name, themes->remembered) == 0;

		if (!is_theme_file(entry))
			continue;
		if (!remembered_refused)
			voe_base_report_error_clear();
		if (!read_one(&themes->entries[themes->count], folder,
			      entry->name, themes->arena, oxanium,
			      pixel_operator)) {
			remembered_refused = remembered_refused || remembered;
			continue;
		}
		if (remembered)
			themes->chosen = themes->count;
		themes->count++;
	}

	if (themes->remembered == NULL || themes->chosen != 0)
		return true;
	// Gone rather than refused: nothing of any other file's is kept to be
	// mistaken for its reason.
	if (!remembered_refused)
		voe_base_report_error_clear();
	return false;
}

const voe_editor_theme *voe_editor_themes_chosen(const voe_editor_themes *themes)
{
	VOE_BASE_ASSERT(themes != NULL && themes->count > 0,
			"asking an unloaded list for its theme");
	return &themes->entries[themes->chosen];
}

bool voe_editor_themes_choose(voe_editor_themes *themes, uint32_t index)
{
	voe_base_arena *scratch;
	const char *settings;
	const char *dir;
	const char *file;
	size_t length;
	char *line;
	bool ok = false;

	VOE_BASE_ASSERT(themes != NULL && index < themes->count,
			"choosing a theme the list does not have");

	themes->chosen = index;
	file = themes->entries[index].file;
	themes->remembered = file;

	scratch = voe_base_arena_new(THEMES_ONE_ARENA);
	settings = voe_platform_folder_settings(scratch);
	if (settings != NULL) {
		dir = voe_platform_path_join(scratch, settings, THEMES_SETTINGS);
		if (ensure_folder(settings, scratch) &&
		    ensure_folder(dir, scratch)) {
			length = file != NULL ? strlen(file) : 0;
			line = voe_base_arena_push(scratch, length + 1);
			if (length > 0)
				memcpy(line, file, length);
			line[length] = '\n';
			ok = voe_platform_file_write(
				voe_platform_path_join(scratch, dir,
						       THEMES_CHOICE),
				(const uint8_t *)line, length + 1, NULL);
		}
	}
	voe_base_arena_destroy(scratch);
	return ok;
}

void voe_editor_themes_destroy(voe_editor_themes *themes)
{
	VOE_BASE_ASSERT(themes != NULL, "destroying no themes");

	for (uint32_t i = 0; i < themes->count; i++)
		if (themes->entries[i].arena != NULL)
			voe_base_arena_destroy(themes->entries[i].arena);
	if (themes->arena != NULL)
		voe_base_arena_destroy(themes->arena);
	*themes = (voe_editor_themes){ 0 };
}
