// The themes folder read into a list, `<settings>/voe3d/theme` read and
// written as its one line, the chosen file re-read once a second, and each
// entry drawn with the two scalars remembered for it. See the header for the
// contract.
#include "themes.h"

#include "theme_scalars.h"

#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <platform/clock.h>
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

// What the remembered file holds for Near white, the one theme with no file
// that is not the default (ADR-0178). No `*.theme` file can have this name.
#define THEMES_NEAR_WHITE "near_white"

// What theme_scalars.h's file names the two themes with no file by (ADR-0197).
#define THEMES_IDENTITY_NEAR_BLACK "near_black"
#define THEMES_IDENTITY_NEAR_WHITE THEMES_NEAR_WHITE

// How many themes with no file start the list: Near black, then Near white.
#define THEMES_BUILT_IN 2u

// Block sizes, not limits: the list's own (paths, the listing, the entries),
// and one theme's (its file's bytes and the strings read out of them) — also
// the scratch a choose writes its one line from.
#define THEMES_LIST_ARENA (16u * 1024u)
#define THEMES_ONE_ARENA (4u * 1024u)

// Seconds between two looks at the chosen file (ADR-0172: within a second).
#define THEMES_CHECK_SECONDS 1.0

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

// Reads `size` bytes of theme file `file`, already pushed into `arena`, into
// `entry`, which then owns `arena`. False, with `entry` untouched and the
// reason already reported, when they will not read; `arena` is the caller's
// to destroy then.
static bool derive_one(voe_editor_theme *entry, const char *file,
		       const uint8_t *bytes, size_t size, voe_base_arena *arena,
		       const voe_text_font *font)
{
	voe_theme theme;
	size_t length = strlen(file);
	char *name;

	if (!voe_theme_read((const char *)bytes, size, arena, &theme))
		return false;
	name = voe_base_arena_push(arena, length + 1);
	memcpy(name, file, length + 1);

	*entry = (voe_editor_theme){
		.name = theme.name,
		.file = name,
		.identity = name,
		.theme = theme,
		.contrast_strength = theme.inputs.contrast_strength,
		.surface_separation = theme.inputs.surface_separation,
		.palette = voe_ui_theme_derive(&theme.inputs, font),
		.bytes = bytes,
		.size = size,
		.arena = arena,
	};
	return true;
}

// Reads one theme file into `entry`, in an arena of its own. False, with that
// arena destroyed and the reason already reported, when it will not read.
static bool read_one(voe_editor_theme *entry, const char *folder,
		     const char *file, voe_base_arena *scratch,
		     const voe_text_font *font)
{
	voe_base_arena *arena = voe_base_arena_new(THEMES_ONE_ARENA);
	const char *path = voe_platform_path_join(scratch, folder, file);
	const uint8_t *bytes;
	size_t size;

	bytes = voe_platform_file_read(path, arena, &size, NULL);
	if (bytes == NULL ||
	    !derive_one(entry, file, bytes, size, arena, font)) {
		voe_base_arena_destroy(arena);
		return false;
	}
	return true;
}

// Forgets the chosen file's last refused bytes.
static void forget_refused(voe_editor_themes *themes)
{
	if (themes->refused != NULL)
		voe_base_arena_destroy(themes->refused);
	themes->refused = NULL;
	themes->refused_bytes = NULL;
	themes->refused_size = 0;
}

// Into the one range both scalars are kept in (ui/theme.h). Written the long
// way round so that a NaN — which no comparison with a bound is true of —
// lands on the minimum instead of passing through.
static float scalar_clamp(float value)
{
	if (!(value > VOE_UI_THEME_SCALAR_MIN))
		return VOE_UI_THEME_SCALAR_MIN;
	return value < VOE_UI_THEME_SCALAR_MAX ? value :
						 VOE_UI_THEME_SCALAR_MAX;
}

// Derives `entry`'s palette from its inputs with the two scalars in force,
// where it stands: the address ui keeps does not move.
static void derive_in_force(voe_editor_theme *entry, const voe_text_font *font)
{
	voe_ui_theme_inputs inputs = entry->theme.inputs;

	inputs.contrast_strength = entry->contrast_strength;
	inputs.surface_separation = entry->surface_separation;
	entry->palette = voe_ui_theme_derive(&inputs, font);
}

// Puts the pair remembered for `entry`'s theme in force and derives it again
// (ADR-0197). Nothing remembered leaves the theme's own two, and the palette
// already derived with them, where they are.
static void apply_remembered(voe_editor_themes *themes,
			     voe_editor_theme *entry)
{
	const voe_editor_theme_scalars_line *line =
		voe_editor_theme_scalars_find(&themes->scalars,
					      entry->identity);

	if (line == NULL)
		return;
	entry->contrast_strength = line->contrast_strength;
	entry->surface_separation = line->surface_separation;
	derive_in_force(entry, themes->font);
}

bool voe_editor_themes_load(voe_editor_themes *themes,
			    const voe_text_font *font)
{
	voe_platform_folder_listing listing = { 0 };
	voe_ui_theme_inputs defaults = voe_ui_theme_default_inputs();
	voe_ui_theme_inputs light;
	const char *settings;
	const char *dir = NULL;
	const char *folder = NULL;
	// True once the remembered file has refused, from which point the
	// report is no longer cleared so its refusal stays the first kept one.
	bool remembered_refused = false;

	VOE_BASE_ASSERT(themes != NULL && themes->arena == NULL,
			"loading themes into a list that is not empty");
	VOE_BASE_ASSERT(font != NULL, "loading themes without a font");

	themes->arena = voe_base_arena_new(THEMES_LIST_ARENA);
	themes->remembered = NULL;
	themes->chosen = 0;
	themes->font = font;
	themes->checked = voe_platform_clock_now();
	themes->scalars_unwritten = false;
	voe_editor_theme_scalars_read(&themes->scalars, themes->arena);

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
		themes->folder = folder;
		if (!ensure_folder(settings, themes->arena) ||
		    !ensure_folder(dir, themes->arena) ||
		    !ensure_folder(folder, themes->arena) ||
		    !voe_platform_folder_list(folder, themes->arena, &listing,
					      NULL))
			listing = (voe_platform_folder_listing){ 0 };
	}

	light = defaults;
	light.mode = VOE_UI_THEME_MODE_LIGHT;
	themes->entries = voe_base_arena_push(
		themes->arena,
		sizeof(voe_editor_theme) * (listing.count + THEMES_BUILT_IN));
	themes->entries[0] = (voe_editor_theme){
		.name = "Near black",
		.identity = THEMES_IDENTITY_NEAR_BLACK,
		.theme = { .name = "Near black",
			   .inputs = defaults,
			   .typeface = VOE_TEXT_TYPEFACE_OXANIUM },
		.contrast_strength = defaults.contrast_strength,
		.surface_separation = defaults.surface_separation,
		.palette = voe_ui_theme_derive(&defaults, font),
	};
	themes->entries[1] = (voe_editor_theme){
		.name = "Near white",
		.identity = THEMES_IDENTITY_NEAR_WHITE,
		.theme = { .name = "Near white",
			   .inputs = light,
			   .typeface = VOE_TEXT_TYPEFACE_OXANIUM },
		.contrast_strength = light.contrast_strength,
		.surface_separation = light.surface_separation,
		.palette = voe_ui_theme_derive(&light, font),
	};
	themes->count = THEMES_BUILT_IN;
	apply_remembered(themes, &themes->entries[0]);
	apply_remembered(themes, &themes->entries[1]);
	if (themes->remembered != NULL &&
	    strcmp(themes->remembered, THEMES_NEAR_WHITE) == 0) {
		themes->chosen = 1;
		themes->remembered = NULL;
	}

	for (uint32_t i = 0; i < listing.count; i++) {
		const voe_platform_folder_entry *entry = &listing.entries[i];
		bool remembered = themes->remembered != NULL &&
				  strcmp(entry->name, themes->remembered) == 0;

		if (!is_theme_file(entry))
			continue;
		if (!remembered_refused)
			voe_base_report_error_clear();
		if (!read_one(&themes->entries[themes->count], folder,
			      entry->name, themes->arena, font)) {
			remembered_refused = remembered_refused || remembered;
			continue;
		}
		apply_remembered(themes, &themes->entries[themes->count]);
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

	if (index != themes->chosen)
		forget_refused(themes);
	themes->chosen = index;
	themes->remembered = themes->entries[index].file;
	// What the one line holds: a file's name, `near_white`, or nothing.
	file = themes->remembered != NULL ? themes->remembered :
	       index == 1		  ? THEMES_NEAR_WHITE :
					    NULL;

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

voe_editor_themes_check_result voe_editor_themes_check(voe_editor_themes *themes)
{
	voe_editor_theme *entry;
	voe_editor_theme fresh;
	voe_base_arena *arena;
	const char *path;
	const uint8_t *bytes;
	const uint8_t *last;
	size_t last_size;
	size_t size;
	double now = voe_platform_clock_now();

	VOE_BASE_ASSERT(themes != NULL && themes->count > 0,
			"checking an unloaded list");

	entry = &themes->entries[themes->chosen];
	if (now - themes->checked < THEMES_CHECK_SECONDS ||
	    entry->file == NULL || themes->folder == NULL)
		return VOE_EDITOR_THEMES_UNCHANGED;
	themes->checked = now;

	arena = voe_base_arena_new(THEMES_ONE_ARENA);
	path = voe_platform_path_join(arena, themes->folder, entry->file);
	last = themes->refused != NULL ? themes->refused_bytes : entry->bytes;
	last_size = themes->refused != NULL ? themes->refused_size :
					      entry->size;
	if (!voe_platform_file_exists(path)) {
		voe_base_arena_destroy(arena);
		return VOE_EDITOR_THEMES_UNCHANGED;
	}
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(path, arena, &size, NULL);
	if (bytes != NULL && size == last_size &&
	    (size == 0 || memcmp(bytes, last, size) == 0)) {
		voe_base_arena_destroy(arena);
		return VOE_EDITOR_THEMES_UNCHANGED;
	}
	if (bytes == NULL ||
	    !derive_one(&fresh, entry->file, bytes, size, arena,
			themes->font)) {
		// Kept only to be compared with; a read that failed outright
		// has no bytes and is tried again next second.
		forget_refused(themes);
		if (bytes != NULL) {
			themes->refused = arena;
			themes->refused_bytes = bytes;
			themes->refused_size = size;
		} else {
			voe_base_arena_destroy(arena);
		}
		return VOE_EDITOR_THEMES_REFUSED;
	}

	forget_refused(themes);
	voe_base_arena_destroy(entry->arena);
	*entry = fresh;
	// The remembered name may have pointed into the arena just destroyed.
	themes->remembered = entry->file;
	apply_remembered(themes, entry);
	return VOE_EDITOR_THEMES_CHANGED;
}

void voe_editor_themes_adjust(voe_editor_themes *themes, uint32_t index,
			      float contrast, float separation)
{
	voe_editor_theme *entry;

	VOE_BASE_ASSERT(themes != NULL && index < themes->count,
			"adjusting a theme the list does not have");

	entry = &themes->entries[index];
	entry->contrast_strength = scalar_clamp(contrast);
	entry->surface_separation = scalar_clamp(separation);
	derive_in_force(entry, themes->font);
	voe_editor_theme_scalars_set(&themes->scalars, themes->arena,
				     entry->identity, entry->contrast_strength,
				     entry->surface_separation);
	themes->scalars_unwritten = true;
}

void voe_editor_themes_reset(voe_editor_themes *themes, uint32_t index)
{
	voe_editor_theme *entry;

	VOE_BASE_ASSERT(themes != NULL && index < themes->count,
			"resetting a theme the list does not have");

	entry = &themes->entries[index];
	entry->contrast_strength = entry->theme.inputs.contrast_strength;
	entry->surface_separation = entry->theme.inputs.surface_separation;
	derive_in_force(entry, themes->font);
	voe_editor_theme_scalars_forget(&themes->scalars, entry->identity);
	themes->scalars_unwritten = true;
}

bool voe_editor_themes_scalars_write(voe_editor_themes *themes)
{
	VOE_BASE_ASSERT(themes != NULL, "writing no list's slider values");

	if (!themes->scalars_unwritten)
		return true;
	if (!voe_editor_theme_scalars_write(&themes->scalars))
		return false;
	themes->scalars_unwritten = false;
	return true;
}

void voe_editor_themes_destroy(voe_editor_themes *themes)
{
	VOE_BASE_ASSERT(themes != NULL, "destroying no themes");

	forget_refused(themes);
	for (uint32_t i = 0; i < themes->count; i++)
		if (themes->entries[i].arena != NULL)
			voe_base_arena_destroy(themes->entries[i].arena);
	if (themes->arena != NULL)
		voe_base_arena_destroy(themes->arena);
	*themes = (voe_editor_themes){ 0 };
}
