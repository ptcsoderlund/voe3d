// The table materials.h describes: the `.material` files found with
// game_tree_find.h's walk, each read with platform and parsed by
// assets/material.h into the next row, the rest of a full table said once.
#include "materials.h"

#include "game_tree_find.h"

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stdio.h>
#include <string.h>

// `<assets>/<path>` parsed into the next row named `Assets/<path>`; false,
// with a line on stderr, when it is too long, will not read or will not parse.
static bool read_one(voe_editor_materials *materials, const char *assets,
		     const char *path, voe_base_arena *scratch)
{
	char *name = materials->paths[materials->count];
	voe_game_material *row = &materials->rows[materials->count];
	voe_base_error error = VOE_BASE_OK;
	const uint8_t *bytes;
	size_t size = 0;
	int length;

	VOE_BASE_ASSERT(materials->count < VOE_EDITOR_MATERIALS,
			"reading a material into a full table");
	VOE_BASE_ASSERT(assets != NULL && path != NULL, "reading no material");
	length = snprintf(name, VOE_ASSETS_MATERIAL_PATH, "Assets/%s", path);
	if (length < 0 || length >= VOE_ASSETS_MATERIAL_PATH) {
		VOE_BASE_ERROR("editor", "left out Assets/%s: its path is too long",
			       path);
		return false;
	}
	bytes = voe_platform_file_read(voe_platform_path_join(scratch, assets,
							      path),
				       scratch, &size, &error);
	if (bytes == NULL ||
	    !voe_assets_material_read((const char *)bytes, size, &row->values,
				      &error)) {
		VOE_BASE_ERROR("editor", "left out %s: %s", name,
			       voe_base_error_string(error));
		return false;
	}
	row->path = name;
	return true;
}

void voe_editor_materials_read(voe_editor_materials *materials,
			       const char *folder, voe_base_arena *scratch)
{
	struct voe_editor_game_tree_found found;
	struct voe_base_arena_mark mark;
	voe_editor_notice why = { 0 };
	const char *assets;

	VOE_BASE_ASSERT(materials != NULL && scratch != NULL,
			"reading materials into no table or with no scratch");
	materials->count = 0;
	if (folder == NULL)
		return;
	mark = voe_base_arena_mark(scratch);
	assets = voe_platform_path_join(scratch, folder, "Assets");
	if (!voe_editor_game_tree_find(assets, ".material", scratch, &found,
				       &why)) {
		VOE_BASE_ERROR("editor", "could not list the materials: %s",
			       why.text);
		found.count = 0;
	}
	for (uint32_t i = 0; i < found.count; i++) {
		struct voe_base_arena_mark one = voe_base_arena_mark(scratch);

		if (materials->count == VOE_EDITOR_MATERIALS) {
			VOE_BASE_ERROR("editor",
				       "left out %u materials past the first %u",
				       found.count - i, VOE_EDITOR_MATERIALS);
			break;
		}
		if (read_one(materials, assets, found.items[i].path, scratch))
			materials->count++;
		voe_base_arena_rewind(scratch, one);
	}
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(materials->count <= VOE_EDITOR_MATERIALS,
			"a table past its room");
}

voe_game_materials
voe_editor_materials_table(const voe_editor_materials *materials)
{
	VOE_BASE_ASSERT(materials != NULL, "a table of no materials");
	VOE_BASE_ASSERT(materials->count <= VOE_EDITOR_MATERIALS,
			"a table past its room");
	return (voe_game_materials){ materials->rows, materials->count };
}

voe_game_material *voe_editor_materials_find(voe_editor_materials *materials,
					     const char *path)
{
	VOE_BASE_ASSERT(materials != NULL && path != NULL,
			"finding no path or in no table");
	VOE_BASE_ASSERT(materials->count <= VOE_EDITOR_MATERIALS,
			"a table past its room");
	for (uint32_t i = 0; i < materials->count; i++)
		if (strcmp(materials->rows[i].path, path) == 0)
			return &materials->rows[i];
	return NULL;
}
