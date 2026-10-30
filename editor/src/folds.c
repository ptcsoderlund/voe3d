// The fold store: folded names kept in order in a fixed array, this frame's
// fold buttons with their names in the frame's arena, and the `fold` lines
// read and written through settings.h's shared line read and keep-other-keys
// write. See the header for the names and when the file is written.
#include "folds.h"

#include "settings.h"

#include <assets/landscape.h>

#include <base/assert.h>
#include <base/report.h>

#include <stdio.h>
#include <string.h>

// What every line this file owns starts with.
#define FOLD_PREFIX "fold "
#define FOLD_PREFIX_LENGTH (sizeof FOLD_PREFIX - 1u)

// The folded name `length` bytes at `name` is at, or folded_count for none.
static uint32_t find(const voe_editor_folds *folds, const char *name,
		     size_t length)
{
	uint32_t i;

	for (i = 0; i < folds->folded_count; i++)
		if (strlen(folds->folded[i]) == length &&
		    memcmp(folds->folded[i], name, length) == 0)
			return i;
	return folds->folded_count;
}

// Keeps `length` bytes at `name` as folded. False, nothing kept, when it is
// empty, too long, already there or the store is full.
static bool add(voe_editor_folds *folds, const char *name, size_t length)
{
	if (length == 0 || length >= VOE_EDITOR_FOLD_NAME ||
	    folds->folded_count == VOE_EDITOR_FOLDS ||
	    find(folds, name, length) != folds->folded_count)
		return false;
	memcpy(folds->folded[folds->folded_count], name, length);
	folds->folded[folds->folded_count][length] = '\0';
	folds->folded_count++;
	return true;
}

// Drops the folded name at `at`, keeping the rest in order.
static void drop(voe_editor_folds *folds, uint32_t at)
{
	VOE_BASE_ASSERT(at < folds->folded_count, "dropping no folded name");
	memmove(folds->folded[at], folds->folded[at + 1],
		(folds->folded_count - at - 1) * sizeof folds->folded[0]);
	folds->folded_count--;
}

// Whether a line is a fold's, which the write replaces.
static bool fold_line(const char *line)
{
	return strncmp(line, FOLD_PREFIX, FOLD_PREFIX_LENGTH) == 0;
}

// One line into the voe_editor_folds at into; other keys' are not its, and a
// Windows line end is cut.
static void fold_take(void *into, const char *line)
{
	voe_editor_folds *folds = into;
	const char *name;

	if (!fold_line(line))
		return;
	name = line + FOLD_PREFIX_LENGTH;
	(void)add(folds, name, strcspn(name, "\r"));
}

// Every folded name as its line, after every other key's, reported at the site
// when the write fails.
static void write_folds(const voe_editor_folds *folds)
{
	char own[VOE_EDITOR_FOLDS * (FOLD_PREFIX_LENGTH + VOE_EDITOR_FOLD_NAME)];
	size_t used = 0;
	uint32_t i;

	for (i = 0; i < folds->folded_count; i++)
		used += (size_t)snprintf(own + used, sizeof own - used,
					 FOLD_PREFIX "%s\n", folds->folded[i]);
	VOE_BASE_ASSERT(used < sizeof own, "the fold lines outgrew their room");
	if (!voe_editor_settings_lines_write(fold_line, own, used))
		VOE_BASE_ERROR("editor",
			       "the folds could not be written to "
			       "<settings>/voe3d/editor_settings");
}

void voe_editor_folds_load(voe_editor_folds *folds)
{
	VOE_BASE_ASSERT(folds != NULL, "loading folds into nothing");
	folds->folded_count = 0;
	voe_editor_settings_lines_read(fold_take, folds);
	VOE_BASE_ASSERT(folds->folded_count <= VOE_EDITOR_FOLDS,
			"more folds than the store holds");
}

bool voe_editor_folds_open(const voe_editor_folds *folds, const char *name)
{
	VOE_BASE_ASSERT(folds != NULL && name != NULL,
			"asking whether nothing is folded");
	return find(folds, name, strlen(name)) == folds->folded_count;
}

void voe_editor_folds_forget(voe_editor_folds *folds, voe_base_arena *frame)
{
	VOE_BASE_ASSERT(folds != NULL && frame != NULL,
			"forgetting folds with no store or no frame");
	folds->shown_count = 0;
	folds->frame = frame;
}

bool voe_editor_folds_begin(voe_ui_context *ui, voe_editor_folds *folds,
			    const char *name, const char *key, uint32_t index)
{
	const bool open = voe_editor_folds_open(folds, name);
	const size_t size = strlen(name) + 1;
	voe_ui_section section;
	char *copy;

	VOE_BASE_ASSERT(ui != NULL && key != NULL, "a fold with no frame or key");
	VOE_BASE_ASSERT(folds->frame != NULL, "a fold before the frame's forget");
	section = voe_ui_section_begin(ui, key, index, open);
	if (folds->shown_count < VOE_EDITOR_FOLDS_SHOWN) {
		copy = voe_base_arena_push(folds->frame, size);
		memcpy(copy, name, size);
		folds->shown[folds->shown_count] = section.fold;
		folds->shown_name[folds->shown_count] = copy;
		folds->shown_count++;
	}
	return open;
}

void voe_editor_folds_read(const voe_ui_context *ui, voe_editor_folds *folds)
{
	bool changed = false;
	const char *name;
	uint32_t at;
	uint32_t i;

	VOE_BASE_ASSERT(ui != NULL && folds != NULL,
			"reading folds with no frame or no store");
	for (i = 0; i < folds->shown_count; i++) {
		if (!voe_ui_button_action(ui, folds->shown[i]).fired)
			continue;
		name = folds->shown_name[i];
		at = find(folds, name, strlen(name));
		if (at < folds->folded_count) {
			drop(folds, at);
			changed = true;
		} else if (add(folds, name, strlen(name))) {
			changed = true;
		}
	}
	if (changed)
		write_folds(folds);
}

void voe_editor_folds_clear(voe_editor_folds *folds, const char *name)
{
	uint32_t at;

	VOE_BASE_ASSERT(folds != NULL && name != NULL, "clearing no fold");
	at = find(folds, name, strlen(name));
	if (at == folds->folded_count)
		return;
	drop(folds, at);
	write_folds(folds);
}

void voe_editor_folds_section_name(char *out, size_t room, const char *word)
{
	int written;

	VOE_BASE_ASSERT(out != NULL && room > 0 && word != NULL,
			"a section's fold name with nowhere to go");
	written = snprintf(out, room, "section %s", word);
	VOE_BASE_ASSERT(written >= 0 && (size_t)written < room,
			"a section's fold name cut short");
}

void voe_editor_folds_kind_name(char *out, size_t room, const char *landscape,
				uint32_t kind)
{
	int written;

	VOE_BASE_ASSERT(out != NULL && room > 0 && landscape != NULL,
			"a kind's fold name with nowhere to go");
	VOE_BASE_ASSERT(kind < VOE_ASSETS_LANDSCAPE_KINDS,
			"a fold for a kind no landscape holds");
	written = snprintf(out, room, "kind %u %s", kind, landscape);
	VOE_BASE_ASSERT(written >= 0 && (size_t)written < room,
			"a kind's fold name cut short");
}
