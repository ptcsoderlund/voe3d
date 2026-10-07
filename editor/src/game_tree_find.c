// The game tree's finder walked — see game_tree_find.h.
//
// One frame per folder level holds its path under Assets/, its listing and the
// next entry to look at; the walk pops a frame when its listing is done. The
// paths are sorted once at the end with qsort and strcmp.
#include "game_tree_find.h"

#include "game_tree.h"

#include <base/assert.h>
#include <base/report.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// One folder level being walked: its path under Assets/ ("" for Assets/), its
// listing and the next entry to look at.
struct find_folder {
	const char *relative;
	voe_platform_folder_listing listing;
	uint32_t next;
};

bool voe_editor_game_tree_folder_there(const char *path, voe_base_arena *arena)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	const char *name = voe_platform_path_name(path);
	voe_platform_folder_listing listing;
	bool there = false;

	VOE_BASE_ASSERT(path != NULL && name != NULL, "looking for no folder");
	if (voe_platform_folder_list(voe_platform_path_parent(arena, path), arena,
				     &listing, NULL))
		for (uint32_t i = 0; i < listing.count && !there; i++)
			there = listing.entries[i].folder &&
				strcmp(listing.entries[i].name, name) == 0;
	voe_base_arena_rewind(arena, mark);
	VOE_BASE_ASSERT(voe_base_arena_mark(arena).used == mark.used,
			"a folder lookup kept memory");
	return there;
}

// Whether name ends in suffix, in any case, and is longer than it.
static bool find_named(const char *name, const char *suffix)
{
	size_t length = strlen(name);
	size_t tail = strlen(suffix);

	VOE_BASE_ASSERT(tail > 0, "matching an empty suffix");
	if (length <= tail)
		return false;
	for (size_t i = 0; i < tail; i++) {
		if (tolower((unsigned char)name[length - tail + i]) !=
		    tolower((unsigned char)suffix[i]))
			return false;
	}
	return true;
}

// path added, the room doubled into arena when it is full.
static void find_path_add(struct voe_editor_game_tree_found *found,
			  voe_base_arena *arena, const char *path)
{
	VOE_BASE_ASSERT(path != NULL && path[0] != '\0', "adding no found path");
	if (found->count == found->room) {
		uint32_t room = found->room == 0 ? 16 : found->room * 2;
		struct voe_editor_game_tree_path *items =
			voe_base_arena_push(arena, room * sizeof *items);

		VOE_BASE_ASSERT(items != NULL, "no room for the found paths");
		if (found->count > 0)
			memcpy(items, found->items, found->count * sizeof *items);
		found->items = items;
		found->room = room;
	}
	found->items[found->count++].path = path;
	VOE_BASE_ASSERT(found->count <= found->room, "found paths outgrew their room");
}

// assets/<relative> listed into frame. False with why naming it.
static bool find_folder_list(const char *assets, const char *relative,
			     voe_base_arena *arena, struct find_folder *frame,
			     voe_editor_notice *why)
{
	const char *path = relative[0] == '\0'
				   ? assets
				   : voe_platform_path_join(arena, assets, relative);

	VOE_BASE_ASSERT(frame != NULL && why != NULL, "listing into no frame");
	*frame = (struct find_folder){ .relative = relative };
	voe_base_report_error_clear();
	if (voe_platform_folder_list(path, arena, &frame->listing, NULL))
		return true;
	voe_editor_notice_from_report(why, path);
	return false;
}

static int find_path_order(const void *a, const void *b)
{
	const struct voe_editor_game_tree_path *left = a;
	const struct voe_editor_game_tree_path *right = b;

	VOE_BASE_ASSERT(left->path != NULL && right->path != NULL,
			"ordering no found path");
	return strcmp(left->path, right->path);
}

// The walk, unsorted.
static bool find_walk(const char *assets, const char *suffix, voe_base_arena *arena,
		      struct voe_editor_game_tree_found *out, voe_editor_notice *why)
{
	struct find_folder stack[VOE_EDITOR_GAME_TREE_DEPTH];
	uint32_t depth = 1;

	VOE_BASE_ASSERT(assets != NULL && out != NULL, "finding files nowhere");
	if (!find_folder_list(assets, "", arena, &stack[0], why))
		return false;
	while (depth > 0) {
		struct find_folder *top = &stack[depth - 1];
		const voe_platform_folder_entry *entry;
		const char *relative;

		if (top->next == top->listing.count) {
			depth--;
			continue;
		}
		entry = &top->listing.entries[top->next++];
		if (entry->hidden)
			continue;
		relative = top->relative[0] == '\0'
				   ? entry->name
				   : voe_editor_game_tree_format(arena, "%s/%s", top->relative,
								 entry->name);
		if (!entry->folder) {
			if (find_named(entry->name, suffix))
				find_path_add(out, arena, relative);
			continue;
		}
		if (depth == VOE_EDITOR_GAME_TREE_DEPTH) {
			voe_editor_notice_set(why, "Assets/%s is deeper than %d folders",
					      relative, VOE_EDITOR_GAME_TREE_DEPTH);
			return false;
		}
		if (!find_folder_list(assets, relative, arena, &stack[depth], why))
			return false;
		depth++;
	}
	VOE_BASE_ASSERT(out->count <= out->room, "found paths outgrew their room");
	return true;
}

bool voe_editor_game_tree_find(const char *assets, const char *suffix,
			       voe_base_arena *arena,
			       struct voe_editor_game_tree_found *out,
			       voe_editor_notice *why)
{
	VOE_BASE_ASSERT(assets != NULL && suffix != NULL, "finding files nowhere");
	VOE_BASE_ASSERT(out != NULL && why != NULL, "finding files into nothing");
	*out = (struct voe_editor_game_tree_found){ 0 };
	if (!voe_editor_game_tree_folder_there(assets, arena))
		return true;
	if (!find_walk(assets, suffix, arena, out, why))
		return false;
	if (out->count > 0)
		qsort(out->items, out->count, sizeof *out->items, find_path_order);
	return true;
}
