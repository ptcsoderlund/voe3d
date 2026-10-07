// The project's scene and prefab texts walked, matched and followed — see
// assets_walk.h.
//
// One walk serves all three calls: a stack of folder levels, each a listing in
// the caller's arena and the next entry to look at, handing back one walked
// file's project-relative name at a time. Each file is read after its name is
// handed back and rewound past once done, so a walk holds its listings and
// names but never two files' text at once.
#include "assets_walk.h"

#include <authoring/paths.h>
#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <ctype.h>
#include <string.h>

// How many folder levels are walked, the project folder one. Deeper is skipped.
#define WALK_DEPTH 16

// The extensions of the files whose text names asset paths. A later own kind
// adds its own here.
static const char *const walked_extensions[] = { ".scene", ".prefab" };

// The top-level folders that hold no authored text.
static const char *const skipped_folders[] = { "Build", "Cache", "Code" };

struct walk_folder {
	const char *relative;
	voe_platform_folder_listing listing;
	uint32_t next;
};

struct walk {
	const char *project;
	struct walk_folder stack[WALK_DEPTH];
	uint32_t depth;
	bool deep_said;
};

enum walk_step { WALK_FILE, WALK_END, WALK_FAILED };

// Whether name ends in one of walked_extensions, any case, and is longer.
static bool walked_named(const char *name)
{
	size_t length = strlen(name);

	VOE_BASE_ASSERT(length > 0, "an entry with no name");
	for (size_t e = 0; e < sizeof walked_extensions / sizeof *walked_extensions; e++) {
		const char *extension = walked_extensions[e];
		size_t tail = strlen(extension);
		size_t i = 0;

		VOE_BASE_ASSERT(tail > 1 && extension[0] == '.', "an extension without its dot");
		if (length <= tail)
			continue;
		while (i < tail && tolower((unsigned char)name[length - tail + i]) == extension[i])
			i++;
		if (i == tail)
			return true;
	}
	return false;
}

static bool skipped_named(const char *name)
{
	VOE_BASE_ASSERT(name != NULL, "no folder name");
	for (size_t i = 0; i < sizeof skipped_folders / sizeof *skipped_folders; i++) {
		if (strcmp(name, skipped_folders[i]) == 0)
			return true;
	}
	return false;
}

// "<folder>/<name>", or name alone under the project folder, into arena.
static const char *relative_join(voe_base_arena *arena, const char *folder, const char *name)
{
	size_t head = strlen(folder);
	size_t tail = strlen(name);
	char *out;

	VOE_BASE_ASSERT(tail > 0, "joining no name");
	if (head == 0)
		return name;
	out = voe_base_arena_push(arena, head + 1 + tail + 1);
	VOE_BASE_ASSERT(out != NULL, "no room for a walked name");
	memcpy(out, folder, head);
	out[head] = '/';
	memcpy(out + head + 1, name, tail + 1);
	return out;
}

// project/<relative> listed onto the stack. False with why naming it.
static bool walk_push(struct walk *walk, const char *relative, voe_base_arena *arena,
		      voe_editor_notice *why)
{
	struct walk_folder *frame = &walk->stack[walk->depth];
	const char *path = relative[0] == '\0'
				   ? walk->project
				   : voe_platform_path_join(arena, walk->project, relative);

	VOE_BASE_ASSERT(walk->depth < WALK_DEPTH, "walking past the stack");
	*frame = (struct walk_folder){ .relative = relative };
	voe_base_report_error_clear();
	if (!voe_platform_folder_list(path, arena, &frame->listing, NULL)) {
		voe_editor_notice_from_report(why, relative[0] == '\0' ? path : relative);
		return false;
	}
	walk->depth++;
	return true;
}

static bool walk_start(struct walk *walk, const char *project, voe_base_arena *arena,
		       voe_editor_notice *why)
{
	VOE_BASE_ASSERT(project != NULL && project[0] != '\0', "walking no project");
	walk->project = project;
	walk->depth = 0;
	walk->deep_said = false;
	return walk_push(walk, "", arena, why);
}

// The next walked file's project-relative name in *relative, the end, or a
// folder that would not list, with why.
static enum walk_step walk_next(struct walk *walk, voe_base_arena *arena,
				const char **relative, voe_editor_notice *why)
{
	VOE_BASE_ASSERT(relative != NULL && why != NULL, "walking into nothing");
	while (walk->depth > 0) {
		struct walk_folder *top = &walk->stack[walk->depth - 1];
		const voe_platform_folder_entry *entry;
		const char *name;

		if (top->next == top->listing.count) {
			walk->depth--;
			continue;
		}
		entry = &top->listing.entries[top->next++];
		if (entry->hidden || (entry->folder ? walk->depth == 1 && skipped_named(entry->name)
						    : !walked_named(entry->name)))
			continue;
		name = relative_join(arena, top->relative, entry->name);
		if (!entry->folder) {
			*relative = name;
			return WALK_FILE;
		}
		if (walk->depth == WALK_DEPTH) {
			if (!walk->deep_said)
				VOE_BASE_WARNING("editor", "%s is deeper than %d folders; not looked in",
						 name, WALK_DEPTH);
			walk->deep_said = true;
			continue;
		}
		if (!walk_push(walk, name, arena, why))
			return WALK_FAILED;
	}
	return WALK_END;
}

// project/<relative> read whole into arena, or NULL with why naming it.
static const char *walked_read(const struct walk *walk, const char *relative,
			       voe_base_arena *arena, size_t *size, voe_editor_notice *why)
{
	const char *path = voe_platform_path_join(arena, walk->project, relative);
	const uint8_t *bytes;

	VOE_BASE_ASSERT(size != NULL && why != NULL, "reading into nothing");
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(path, arena, size, NULL);
	if (bytes == NULL)
		voe_editor_notice_from_report(why, relative);
	return (const char *)bytes;
}

void voe_editor_assets_users(const char *project, const char *path,
			     voe_base_arena *arena, voe_editor_assets_used *out)
{
	struct walk walk;
	voe_editor_notice why;
	const char *relative = NULL;

	VOE_BASE_ASSERT(path != NULL && path[0] != '\0', "asking who uses no path");
	VOE_BASE_ASSERT(arena != NULL && out != NULL, "users into nothing");
	*out = (voe_editor_assets_used){ 0 };
	if (!walk_start(&walk, project, arena, &why))
		return;
	while (walk_next(&walk, arena, &relative, &why) == WALK_FILE) {
		struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
		size_t size = 0;
		const char *text = walked_read(&walk, relative, arena, &size, &why);
		bool named = text != NULL && voe_authoring_paths_named(text, size, path);

		voe_base_arena_rewind(arena, mark);
		if (!named)
			continue;
		if (out->count < VOE_EDITOR_ASSETS_USERS_KEPT)
			out->names[out->count] = relative;
		out->count++;
	}
}

// One walked file followed in scratch, and written when write and it changed.
// False with why naming it.
static bool follow_one(const struct walk *walk, const char *relative, const char *from,
		       const char *to, bool write, voe_base_arena *scratch,
		       voe_editor_notice *why)
{
	voe_authoring_paths_followed followed;
	size_t size = 0;
	const char *text = walked_read(walk, relative, scratch, &size, why);

	VOE_BASE_ASSERT(from != NULL && to != NULL, "following no path");
	if (text == NULL)
		return false;
	voe_base_report_error_clear();
	if (!voe_authoring_paths_follow(text, size, from, to, scratch, &followed)) {
		voe_editor_notice_from_report(why, relative);
		return false;
	}
	if (followed.changed == 0)
		return true;
	if (followed.longest >= VOE_EDITOR_ASSETS_PATH_ROOM) {
		voe_editor_notice_set(why, "%s: a followed path would be %zu bytes, %d or more",
				      relative, followed.longest, VOE_EDITOR_ASSETS_PATH_ROOM);
		return false;
	}
	if (!write)
		return true;
	voe_base_report_error_clear();
	if (voe_platform_file_write(voe_platform_path_join(scratch, walk->project, relative),
				    (const uint8_t *)followed.text.text, followed.text.size, NULL))
		return true;
	voe_editor_notice_from_report(why, relative);
	return false;
}

// Every walked file followed, written when write. scratch is rewound before
// return.
static bool follow_all(const char *project, const char *from, const char *to, bool write,
		       voe_base_arena *scratch, voe_editor_notice *why)
{
	struct voe_base_arena_mark start;
	struct walk walk;
	const char *relative = NULL;
	enum walk_step step = WALK_FAILED;
	bool ok = true;

	VOE_BASE_ASSERT(scratch != NULL && why != NULL, "following into nothing");
	VOE_BASE_ASSERT(from != NULL && to != NULL && from[0] != '\0' && to[0] != '\0',
			"following an empty path");
	start = voe_base_arena_mark(scratch);
	if (walk_start(&walk, project, scratch, why)) {
		while (ok && (step = walk_next(&walk, scratch, &relative, why)) == WALK_FILE) {
			struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);

			ok = follow_one(&walk, relative, from, to, write, scratch, why);
			voe_base_arena_rewind(scratch, mark);
		}
		ok = ok && step == WALK_END;
	} else {
		ok = false;
	}
	voe_base_arena_rewind(scratch, start);
	return ok;
}

bool voe_editor_assets_follow_check(const char *project, const char *from, const char *to,
				    voe_base_arena *scratch, voe_editor_notice *why)
{
	return follow_all(project, from, to, false, scratch, why);
}

bool voe_editor_assets_follow_write(const char *project, const char *from, const char *to,
				    voe_base_arena *scratch, voe_editor_notice *why)
{
	return follow_all(project, from, to, true, scratch, why);
}
