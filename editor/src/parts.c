// The table parts.h describes: the `.parts` files found with
// game_tree_find.h's walk, each read with platform and parsed by
// assets/parts.h into the next row named by its `.glb`, the rest of a full
// table said once.
#include "parts.h"

#include "game_tree_find.h"

#include <assets/parts.h>

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stdio.h>
#include <string.h>

// The ending game_tree_find.h matches, whose bytes the row's `.glb` replaces.
#define PARTS_ENDING ".parts"

// `<assets>/<path>` parsed into the next row named `Assets/<stem>.glb`; false,
// with a line on stderr, when it is too long, will not read or will not parse.
static bool read_one(voe_editor_parts *parts, const char *assets,
		     const char *path, voe_base_arena *scratch)
{
	char *name = parts->paths[parts->count];
	voe_game_model_parts *row = &parts->rows[parts->count];
	const size_t stem = strlen(path) - strlen(PARTS_ENDING);
	voe_base_error error = VOE_BASE_OK;
	const uint8_t *bytes;
	size_t size = 0;
	int length;

	VOE_BASE_ASSERT(parts->count < VOE_EDITOR_PARTS,
			"reading parts into a full table");
	VOE_BASE_ASSERT(assets != NULL && strlen(path) >= strlen(PARTS_ENDING),
			"reading parts with no ending");
	length = snprintf(name, VOE_3D_MODEL_PATH, "Assets/%.*s.glb", (int)stem,
			  path);
	if (length < 0 || length >= VOE_3D_MODEL_PATH) {
		VOE_BASE_ERROR("editor", "left out Assets/%s: its path is too long",
			       path);
		return false;
	}
	bytes = voe_platform_file_read(voe_platform_path_join(scratch, assets,
							      path),
				       scratch, &size, &error);
	if (bytes == NULL || !voe_assets_parts_read((const char *)bytes, size,
						    &row->parts, &error)) {
		VOE_BASE_ERROR("editor", "left out Assets/%s: %s", path,
			       voe_base_error_string(error));
		return false;
	}
	row->path = name;
	return true;
}

void voe_editor_parts_read(voe_editor_parts *parts, const char *folder,
			   voe_base_arena *scratch)
{
	struct voe_editor_game_tree_found found;
	struct voe_base_arena_mark mark;
	voe_editor_notice why = { 0 };
	const char *assets;

	VOE_BASE_ASSERT(parts != NULL && scratch != NULL,
			"reading parts into no table or with no scratch");
	parts->count = 0;
	if (folder == NULL)
		return;
	mark = voe_base_arena_mark(scratch);
	assets = voe_platform_path_join(scratch, folder, "Assets");
	if (!voe_editor_game_tree_find(assets, PARTS_ENDING, scratch, &found,
				       &why)) {
		VOE_BASE_ERROR("editor", "could not list the parts: %s",
			       why.text);
		found.count = 0;
	}
	for (uint32_t i = 0; i < found.count; i++) {
		struct voe_base_arena_mark one = voe_base_arena_mark(scratch);

		if (parts->count == VOE_EDITOR_PARTS) {
			VOE_BASE_ERROR("editor",
				       "left out %u parts files past the first %u",
				       found.count - i, VOE_EDITOR_PARTS);
			break;
		}
		if (read_one(parts, assets, found.items[i].path, scratch))
			parts->count++;
		voe_base_arena_rewind(scratch, one);
	}
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(parts->count <= VOE_EDITOR_PARTS,
			"a table past its room");
}

voe_game_parts voe_editor_parts_table(const voe_editor_parts *parts)
{
	VOE_BASE_ASSERT(parts != NULL, "a table of no parts");
	VOE_BASE_ASSERT(parts->count <= VOE_EDITOR_PARTS,
			"a table past its room");
	return (voe_game_parts){ parts->rows, parts->count };
}
