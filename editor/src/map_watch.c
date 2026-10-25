// The map watch map_watch.h describes: each look marks the paths the rows
// still name, stamps each once under the folder, notes a new one, reports a
// changed one while room is left, then lets the unmarked paths go in place.
#include "map_watch.h"

#include <base/assert.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stdio.h>
#include <string.h>

// `path`'s slot in `watch`, `count` when it holds none: a scan.
static uint32_t slot_of(const voe_editor_map_watch *watch, const char *path)
{
	uint32_t at = 0;

	VOE_BASE_ASSERT(watch != NULL && path != NULL, "finding no path");
	while (at < watch->count && strcmp(watch->paths[at], path) != 0)
		at++;
	VOE_BASE_ASSERT(at <= watch->count, "a slot past the watch's paths");
	return at;
}

// `path` marked and stamped, once a look however many rows name it: noted
// when new, its stamp taken when it changed and `room_left`. True for a
// change to report.
static bool look_one(voe_editor_map_watch *watch, const char *folder,
		     const char *path, bool room_left, voe_base_arena *scratch)
{
	uint32_t at = slot_of(watch, path);
	struct voe_base_arena_mark mark;
	uint64_t stamp = 0;

	VOE_BASE_ASSERT(folder != NULL && scratch != NULL,
			"stamping a map in no folder or with no scratch");
	VOE_BASE_ASSERT(path[0] != '\0', "stamping no map");
	if (at < watch->count && watch->seen[at])
		return false;
	mark = voe_base_arena_mark(scratch);
	if (!voe_platform_file_stamp(voe_platform_path_join(scratch, folder,
							    path),
				     &stamp))
		stamp = 0;
	voe_base_arena_rewind(scratch, mark);
	if (at == watch->count) {
		VOE_BASE_ASSERT(at < VOE_EDITOR_MAP_WATCH,
				"more maps named than the watch's room");
		snprintf(watch->paths[at], VOE_ASSETS_MATERIAL_PATH, "%s", path);
		watch->stamps[at] = stamp;
		watch->seen[at] = true;
		watch->count++;
		return false;
	}
	watch->seen[at] = true;
	if (stamp == watch->stamps[at] || !room_left)
		return false;
	watch->stamps[at] = stamp;
	return true;
}

// Every path the look did not mark let go, the rest kept in order.
static void let_go(voe_editor_map_watch *watch)
{
	uint32_t kept = 0;

	VOE_BASE_ASSERT(watch != NULL, "letting go in no watch");
	VOE_BASE_ASSERT(watch->count <= VOE_EDITOR_MAP_WATCH,
			"a watch past its room");
	for (uint32_t i = 0; i < watch->count; i++) {
		if (!watch->seen[i])
			continue;
		if (kept != i) {
			memcpy(watch->paths[kept], watch->paths[i],
			       VOE_ASSETS_MATERIAL_PATH);
			watch->stamps[kept] = watch->stamps[i];
		}
		kept++;
	}
	watch->count = kept;
}

uint32_t voe_editor_map_watch_look(voe_editor_map_watch *watch,
				   const char *folder, voe_game_materials table,
				   voe_editor_map_changes *changed,
				   voe_base_arena *scratch)
{
	VOE_BASE_ASSERT(watch != NULL && folder != NULL && scratch != NULL,
			"looking at maps with no watch, folder or scratch");
	VOE_BASE_ASSERT(changed != NULL, "reporting changed maps into nowhere");
	VOE_BASE_ASSERT(table.materials != NULL || table.count == 0,
			"a table with rows and none to read");
	memset(watch->seen, 0, sizeof watch->seen);
	changed->count = 0;
	for (uint32_t i = 0; i < table.count; i++) {
		const voe_assets_material_file *values =
			&table.materials[i].values;
		const char *maps[4] = { values->colormap, values->normalmap,
					values->ormmap, values->emissionmap };

		for (uint32_t m = 0; m < 4; m++)
			if (maps[m][0] != '\0' &&
			    look_one(watch, folder, maps[m],
				     changed->count < VOE_EDITOR_MAP_CHANGES,
				     scratch))
				changed->paths[changed->count++] = maps[m];
	}
	let_go(watch);
	VOE_BASE_ASSERT(changed->count <= VOE_EDITOR_MAP_CHANGES,
			"more changed maps than the room");
	return changed->count;
}

void voe_editor_map_watch_clear(voe_editor_map_watch *watch)
{
	VOE_BASE_ASSERT(watch != NULL, "clearing no watch");
	watch->count = 0;
	VOE_BASE_ASSERT(watch->count == 0, "a cleared watch holds paths");
}
