// The table trees.h describes: the `.tree` files found with game_tree_find.h's
// walk, each read with platform and parsed leniently by assets/tree.h into the
// next row, the rest of a full table said once.
#include "trees.h"

#include "game_tree_find.h"

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stdio.h>
#include <string.h>

// `<assets>/<path>` parsed into the next row named `Assets/<path>`, unmarked;
// false, with a line on stderr, when it is too long, will not read or will
// not parse.
static bool read_one(voe_editor_trees *trees, const char *assets,
		     const char *path, voe_base_arena *scratch)
{
	char *name = trees->paths[trees->count];
	voe_game_tree *row = &trees->rows[trees->count];
	voe_base_error error = VOE_BASE_OK;
	const uint8_t *bytes;
	size_t size = 0;
	int length;

	VOE_BASE_ASSERT(trees->count < VOE_EDITOR_TREES,
			"reading a species into a full table");
	VOE_BASE_ASSERT(assets != NULL && path != NULL, "reading no species");
	length = snprintf(name, VOE_ASSETS_LANDSCAPE_KIND_PATH, "Assets/%s",
			  path);
	if (length < 0 || length >= VOE_ASSETS_LANDSCAPE_KIND_PATH) {
		VOE_BASE_ERROR("editor", "left out Assets/%s: its path is too long",
			       path);
		return false;
	}
	bytes = voe_platform_file_read(voe_platform_path_join(scratch, assets,
							      path),
				       scratch, &size, &error);
	if (bytes == NULL || !voe_assets_tree_read((const char *)bytes, size,
						   &row->values, &error)) {
		VOE_BASE_ERROR("editor", "left out %s: %s", name,
			       voe_base_error_string(error));
		return false;
	}
	row->path = name;
	trees->regrow[trees->count] = false;
	return true;
}

void voe_editor_trees_read(voe_editor_trees *trees, const char *folder,
			   voe_base_arena *scratch)
{
	struct voe_editor_game_tree_found found;
	struct voe_base_arena_mark mark;
	voe_editor_notice why = { 0 };
	const char *assets;

	VOE_BASE_ASSERT(trees != NULL && scratch != NULL,
			"reading species into no table or with no scratch");
	trees->count = 0;
	if (folder == NULL)
		return;
	mark = voe_base_arena_mark(scratch);
	assets = voe_platform_path_join(scratch, folder, "Assets");
	if (!voe_editor_game_tree_find(assets, ".tree", scratch, &found,
				       &why)) {
		VOE_BASE_ERROR("editor", "could not list the species: %s",
			       why.text);
		found.count = 0;
	}
	for (uint32_t i = 0; i < found.count; i++) {
		struct voe_base_arena_mark one = voe_base_arena_mark(scratch);

		if (trees->count == VOE_EDITOR_TREES) {
			VOE_BASE_ERROR("editor",
				       "left out %u species past the first %u",
				       found.count - i, VOE_EDITOR_TREES);
			break;
		}
		if (read_one(trees, assets, found.items[i].path, scratch))
			trees->count++;
		voe_base_arena_rewind(scratch, one);
	}
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(trees->count <= VOE_EDITOR_TREES,
			"a table past its room");
}

voe_game_trees voe_editor_trees_table(const voe_editor_trees *trees)
{
	VOE_BASE_ASSERT(trees != NULL, "a table of no species");
	VOE_BASE_ASSERT(trees->count <= VOE_EDITOR_TREES,
			"a table past its room");
	return (voe_game_trees){ trees->rows, trees->count };
}

voe_game_tree *voe_editor_trees_find(voe_editor_trees *trees, const char *path)
{
	VOE_BASE_ASSERT(trees != NULL && path != NULL,
			"finding no path or in no table");
	VOE_BASE_ASSERT(trees->count <= VOE_EDITOR_TREES,
			"a table past its room");
	for (uint32_t i = 0; i < trees->count; i++)
		if (strcmp(trees->rows[i].path, path) == 0)
			return &trees->rows[i];
	return NULL;
}
