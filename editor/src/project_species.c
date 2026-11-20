// A species opened in the level's place, its variation stepped, and whether
// one is open: the three calls project.h's A SPECIES RIDES IT point describes.
// The set-aside is the open prefab's (0515 point 6), so Back is
// voe_editor_project_prefab_back and Play and Ship are refused as for a prefab.
//
// THE SPECIES FILE IS NOT READ HERE: it is no scene text, and the store grows
// its variations (models.h). The world is emptied through
// voe_editor_project_scene_set with no text, the one call that empties a
// world, then the untitled scene's sun (project_prefab.h) and the tree's
// model row are added straight in, as the untitled scene's rows are.
//
// A STEPPED VARIATION GOES THROUGH THE MODEL'S INTENT (3d/model_component.h),
// so the row wears it from the next model system run. The tree's row is found
// by its id, so a sun moved or a thing added in the view does not confuse it.
#include "project.h"
#include "project_prefab.h"

#include <3d/model_component.h>

#include <assets/tree.h>

#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/transform_system.h>

#include <stdint.h>
#include <string.h>

// The arena the set-aside level lives in: project_prefab.c's, a block size,
// not a limit.
#define SPECIES_ARENA (1u * 1024u * 1024u)

// The ids of the species world's two rows.
#define SUN_ID 1
#define TREE_ID 2

// `Tree`, TREE_ID, at the origin with the transform's default row (unturned,
// scale one), wearing `model`.
static void tree_add(voe_ecs_world *world, const voe_3d_model *model)
{
	const voe_scene_transform *origin = voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_scene_transform_key));
	voe_scene_identity identity = { .id = TREE_ID, .name = "Tree" };
	voe_ecs_entity tree;

	VOE_BASE_ASSERT(origin != NULL && model != NULL,
			"a tree with no default transform or no model");
	VOE_BASE_ASSERT(voe_ecs_entity_create(world, &tree),
			"a project's world is too small for a species");
	VOE_BASE_ASSERT(voe_scene_identity_add(world, tree, identity),
			"a project's identity table is too small for a species");
	VOE_BASE_ASSERT(voe_scene_transform_add(world, tree, *origin),
			"a project's transform table is too small for a species");
	VOE_BASE_ASSERT(voe_3d_model_add(world, tree, *model),
			"a project's model table is too small for a species");
}

bool voe_editor_project_species_open(voe_editor_project *project,
				     const char *path, uint32_t variation,
				     voe_editor_notice *why)
{
	const voe_3d_model *row;
	voe_base_arena *arena;
	voe_authoring_text level;
	voe_3d_model model;
	bool emptied;

	VOE_BASE_ASSERT(project != NULL && path != NULL && why != NULL,
			"opening a species with no project, path or notice");
	VOE_BASE_ASSERT(project->folder != NULL, "opening a species untitled");
	VOE_BASE_ASSERT(project->prefab[0] == '\0' &&
				project->level_arena == NULL,
			"opening a species while a prefab is open");

	row = voe_ecs_component_default(
		project->world,
		voe_ecs_component_type(project->world, &voe_3d_model_key));
	VOE_BASE_ASSERT(row != NULL, "a model with no default row");
	model = *row;
	if (strlen(path) >= sizeof project->prefab ||
	    !voe_assets_tree_variation_path(model.path, sizeof model.path, path,
					    variation)) {
		voe_editor_notice_set(why, "The species' path is too long.");
		return false;
	}
	arena = voe_base_arena_new(SPECIES_ARENA);
	voe_base_report_error_clear();
	if (!voe_editor_project_scene_text(project, arena, &level)) {
		voe_editor_notice_from_report(why, "the scene");
		voe_base_arena_destroy(arena);
		return false;
	}
	emptied = voe_editor_project_scene_set(project, "", 0, why);
	VOE_BASE_ASSERT(emptied, "a world that would not empty");
	voe_editor_project_sun_add(project->world, SUN_ID);
	tree_add(project->world, &model);

	memcpy(project->prefab, path, strlen(path) + 1);
	project->level_arena = arena;
	project->level_text = level.text;
	project->level_size = level.size;
	project->level_unsaved = project->unsaved;
	project->unsaved = false;
	VOE_BASE_ASSERT(voe_editor_project_species_is(project),
			"an opened species that is not one");
	return true;
}

bool voe_editor_project_species_variation(voe_editor_project *project,
					  uint32_t v)
{
	const voe_3d_model *rows;
	const voe_ecs_entity *entities;
	uint32_t count;

	VOE_BASE_ASSERT(project != NULL && voe_editor_project_species_is(project),
			"stepping the variation with no species open");
	rows = voe_3d_model_rows(project->world);
	entities = voe_3d_model_entities(project->world);
	count = voe_3d_model_count(project->world);
	VOE_BASE_ASSERT(rows != NULL || count == 0, "model entities with no rows");
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_identity *identity =
			voe_scene_identity_get(project->world, entities[i]);
		voe_3d_model_intent intent = { .entity = entities[i],
					       .model = rows[i] };

		if (identity == NULL || identity->id != TREE_ID)
			continue;
		return voe_assets_tree_variation_path(intent.model.path,
						      sizeof intent.model.path,
						      project->prefab, v) &&
		       voe_3d_model_submit(project->world, intent);
	}
	return false;
}

bool voe_editor_project_species_is(const voe_editor_project *project)
{
	VOE_BASE_ASSERT(project != NULL, "asking no project for a species");
	VOE_BASE_ASSERT(memchr(project->prefab, '\0', sizeof project->prefab) !=
				NULL,
			"an open prefab's path with no end");
	return project->prefab[0] != '\0' &&
	       voe_assets_tree_path_is(project->prefab);
}
