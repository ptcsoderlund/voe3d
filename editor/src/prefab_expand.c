// Every placed copy in a world expanded from its file (prefabs.h).
//
// Constraints: roots are found by a scan of the prefab table per root, bounded
// by the table's count when the call began; the scratch is rewound after each.
#include "prefabs.h"

#include <authoring/prefab.h>

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <ecs/component.h>

#include <platform/file.h>
#include <platform/path.h>

#include <scene/identity_component.h>
#include <scene/prefab_component.h>
#include <scene/transform_component.h>

#include <stdint.h>
#include <string.h>

// One above the largest identity id in world, 1 in a world with none.
static uint64_t id_above_all(const voe_ecs_world *world)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(world);
	uint32_t count = voe_scene_identity_count(world);
	uint64_t largest = 0;
	uint32_t i;

	VOE_BASE_ASSERT(count == 0 || rows != NULL, "identities with no rows");
	for (i = 0; i < count; i++)
		if (rows[i].id > largest)
			largest = rows[i].id;
	VOE_BASE_ASSERT(largest < UINT64_MAX, "an identity id with none above it");
	return largest + 1;
}

// The root with a prefab row, no part row and the smallest identity id (0 for
// one with no identity); false when none is left.
static bool next_root(const voe_ecs_world *world, voe_ecs_type prefab_type,
		      voe_ecs_entity *out)
{
	const voe_ecs_entity *entities =
		voe_ecs_component_entities(world, prefab_type);
	uint32_t count = voe_ecs_component_count(world, prefab_type);
	uint64_t best = 0;
	bool found = false;
	uint32_t i;

	VOE_BASE_ASSERT(out != NULL, "finding a root with nowhere to put it");
	VOE_BASE_ASSERT(count == 0 || entities != NULL, "prefab rows with no entities");
	for (i = 0; i < count; i++) {
		const voe_scene_identity *identity;
		uint64_t id;

		if (voe_scene_prefab_part_get(world, entities[i]) != NULL)
			continue;
		identity = voe_scene_identity_get(world, entities[i]);
		id = identity != NULL ? identity->id : 0;
		if (!found || id < best) {
			best = id;
			*out = entities[i];
			found = true;
		}
	}
	return found;
}

// Reads `<folder>/<path>` onto root from first_id. NULL with *out_next_id the
// id after the last used, or the category of why it did not.
static const char *read_onto(voe_ecs_world *world, voe_ecs_entity root,
			     const char *folder, const char *path,
			     uint64_t first_id, voe_base_arena *scratch,
			     uint64_t *out_next_id)
{
	voe_base_error error;
	const uint8_t *bytes;
	size_t size;
	const char *kept;
	const char *phrase;

	VOE_BASE_ASSERT(folder != NULL && path != NULL, "reading no prefab file");
	VOE_BASE_ASSERT(out_next_id != NULL, "nowhere to put the next id");

	// A HAND-WRITTEN SCENE CAN PLACE A COPY WITH NO TRANSFORM; the reader
	// asserts on one, so it is refused here.
	if (voe_scene_transform_get(world, root) == NULL)
		return "the placed copy has no transform";

	voe_base_report_error_clear();
	bytes = voe_platform_file_read(
		voe_platform_path_join(scratch, folder, path), scratch, &size,
		&error);
	if (bytes == NULL)
		return voe_base_error_string(error);

	voe_base_report_error_clear();
	if (voe_authoring_prefab_read((const char *)bytes, size, world, root,
				      first_id, scratch, out_next_id))
		return NULL;
	// THE CATEGORY IS THE KEPT ERROR'S LAST PHRASE, as models.c says a
	// broken model: the file is already named by the notice.
	kept = voe_base_report_error_first();
	if (kept == NULL)
		return "refused";
	phrase = strrchr(kept, ':');
	if (phrase == NULL)
		return kept;
	return phrase[1] == ' ' ? phrase + 2 : phrase + 1;
}

void voe_editor_prefabs_expand(voe_ecs_world *world, const char *folder,
			       voe_base_arena *scratch, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark;
	voe_ecs_type prefab_type;
	voe_ecs_type part_type;
	uint64_t next_id;
	uint32_t bound;
	uint32_t i;

	VOE_BASE_ASSERT(world != NULL && scratch != NULL,
			"expanding prefabs with no world or scratch");
	VOE_BASE_ASSERT(why != NULL, "expanding prefabs with nowhere to say why");
	if (folder == NULL)
		return;

	prefab_type = voe_ecs_component_type(world, &voe_scene_prefab_key);
	part_type = voe_ecs_component_type(world, &voe_scene_prefab_part_key);
	mark = voe_base_arena_mark(scratch);
	next_id = id_above_all(world);
	bound = voe_ecs_component_count(world, prefab_type);

	for (i = 0; i < bound; i++) {
		char path[VOE_SCENE_PREFAB_PATH];
		voe_ecs_entity root;
		const char *category;
		uint64_t after = next_id;
		voe_scene_prefab_part part;

		if (!next_root(world, prefab_type, &root))
			break;
		memcpy(path, voe_scene_prefab_get(world, root)->path, sizeof path);
		path[sizeof path - 1] = '\0';

		category = read_onto(world, root, folder, path, next_id, scratch,
				     &after);
		voe_base_arena_rewind(scratch, mark);
		if (category == NULL) {
			next_id = after;
			continue;
		}

		// A refused read may leave the world half-made, so the next
		// id is found again rather than trusted.
		voe_editor_notice_set(why, "Could not read %s: %s", path, category);
		next_id = id_above_all(world);
		part = (voe_scene_prefab_part){ .instance = root };
		if (voe_scene_prefab_part_get(world, root) == NULL &&
		    !voe_ecs_component_add(world, part_type, root, &part))
			break;
	}
	VOE_BASE_ASSERT(next_id > 0, "an expansion that used id 0");
}
