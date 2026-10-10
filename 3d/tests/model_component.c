// The model component and its drain: that the description is one CHAR field
// `path` of 128 bytes, a BOOL `cast_shadows`, a FLOAT32 `fade` and eight
// `materials` paths, that the
// default row casts unfaded, an intent with cast_shadows false lands and one
// with fade 0.5 reads back after a run, that a row added and then given a new path by an intent
// reads that path back after a run, that a dead entity's intent is dropped,
// and that a path or a material with no end is cut to 127 bytes. Needs no
// graphics card.
//
// THE DESCRIPTION IS SWITCHED ON HERE, WHATEVER THE BUILD SAID, for the reason
// scene/tests/identity.c gives: check.cmake builds without descriptions, and a
// check that followed the build would never run on the run that gates a card.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <3d/model_component.h>

#include <base/arena.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stddef.h>
#include <string.h>

#define SCRATCH (64 * 1024)
#define ENTITIES 4

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = ENTITIES,
		.component_types = 2,
		.intent_types = 2,
		.structure_requests = 4 * ENTITIES,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	// Transform first: a model's registration names what it needs.
	voe_scene_transform_register(world, ENTITIES);
	voe_3d_model_register(world, ENTITIES);
	return world;
}

static voe_ecs_entity modelled(voe_ecs_world *world, const char *path)
{
	voe_ecs_entity entity = { 0 };
	voe_3d_model model = { .cast_shadows = true };

	strcpy(model.path, path);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_3d_model_add(world, entity, model));
	return entity;
}

static void the_description_is_a_path_cast_shadows_and_fade(void)
{
	const voe_base_struct_description *description =
		voe_3d_model_description();

	VOE_TEST_CHECK(strcmp(description->name, "voe_3d_model") == 0);
	VOE_TEST_CHECK_INT(description->field_count, 4);
	if (description->field_count != 4)
		return;
	VOE_TEST_CHECK(strcmp(description->fields[0].name, "path") == 0);
	VOE_TEST_CHECK_INT(description->fields[0].kind, VOE_BASE_FIELD_CHAR);
	VOE_TEST_CHECK_INT(description->fields[0].count, VOE_3D_MODEL_PATH);
	VOE_TEST_CHECK_INT((long long)description->fields[0].offset,
			   (long long)offsetof(voe_3d_model, path));
	VOE_TEST_CHECK(strcmp(description->fields[1].name, "cast_shadows") ==
		       0);
	VOE_TEST_CHECK_INT(description->fields[1].kind, VOE_BASE_FIELD_BOOL);
	VOE_TEST_CHECK_INT((long long)description->fields[1].offset,
			   (long long)offsetof(voe_3d_model, cast_shadows));
	VOE_TEST_CHECK(strcmp(description->fields[2].name, "fade") == 0);
	VOE_TEST_CHECK_INT(description->fields[2].kind, VOE_BASE_FIELD_FLOAT32);
	VOE_TEST_CHECK_INT((long long)description->fields[2].offset,
			   (long long)offsetof(voe_3d_model, fade));
	VOE_TEST_CHECK(strcmp(description->fields[3].name, "materials") == 0);
	VOE_TEST_CHECK_INT(description->fields[3].kind, VOE_BASE_FIELD_CHAR);
	VOE_TEST_CHECK_INT(description->fields[3].count,
			   VOE_3D_MODEL_MATERIALS * VOE_3D_MODEL_PATH);
}

// The default row is the empty path, casting, unfaded; an intent with
// cast_shadows false lands as it is.
static void the_default_casts_and_an_intent_turns_it_off(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	const voe_3d_model *row = voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_3d_model_key));
	voe_ecs_entity entity = modelled(world, "Assets/hull.glb");
	voe_3d_model_intent intent = { .entity = entity,
				       .model = { .path = "Assets/hull.glb",
						  .cast_shadows = false } };

	VOE_TEST_CHECK(row != NULL && row->path[0] == '\0' && row->cast_shadows);
	VOE_TEST_CHECK(row != NULL && row->fade == 0.0f);
	VOE_TEST_CHECK(voe_3d_model_submit(world, intent));
	voe_3d_model_system_run(world);
	VOE_TEST_CHECK(!voe_3d_model_get(world, entity)->cast_shadows);
	VOE_TEST_CHECK(strcmp(voe_3d_model_get(world, entity)->path,
			      "Assets/hull.glb") == 0);
	voe_base_arena_clear(arena);
}

static void a_submitted_fade_reads_back(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = modelled(world, "Assets/hull.glb");
	voe_3d_model_intent intent = { .entity = entity,
				       .model = { .path = "Assets/hull.glb",
						  .cast_shadows = true,
						  .fade = 0.5f } };

	VOE_TEST_CHECK(voe_3d_model_submit(world, intent));
	voe_3d_model_system_run(world);
	VOE_TEST_CHECK_FLOAT(voe_3d_model_get(world, entity)->fade, 0.5f, 0.0f);
	voe_base_arena_clear(arena);
}

static void a_submitted_path_reads_back(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = modelled(world, "Assets/hull.glb");
	voe_3d_model_intent intent = { .entity = entity };

	strcpy(intent.model.path, "Assets/deck.glb");
	VOE_TEST_CHECK(voe_3d_model_submit(world, intent));
	VOE_TEST_CHECK(strcmp(voe_3d_model_get(world, entity)->path,
			      "Assets/hull.glb") == 0);
	voe_3d_model_system_run(world);
	VOE_TEST_CHECK_INT(voe_3d_model_count(world), 1);
	VOE_TEST_CHECK(strcmp(voe_3d_model_rows(world)[0].path,
			      "Assets/deck.glb") == 0);
	VOE_TEST_CHECK(voe_3d_model_entities(world)[0].index == entity.index);
	voe_base_arena_clear(arena);
}

static void a_dead_entitys_intent_is_dropped(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity kept = modelled(world, "Assets/hull.glb");
	voe_ecs_entity dead = modelled(world, "Assets/hull.glb");
	voe_3d_model_intent intent = { .entity = dead };

	voe_ecs_entity_destroy(world, dead);
	strcpy(intent.model.path, "Assets/deck.glb");
	VOE_TEST_CHECK(voe_3d_model_submit(world, intent));
	voe_3d_model_system_run(world);
	VOE_TEST_CHECK(voe_3d_model_get(world, dead) == NULL);
	VOE_TEST_CHECK(strcmp(voe_3d_model_get(world, kept)->path,
			      "Assets/hull.glb") == 0);
	voe_base_arena_clear(arena);
}

static void a_path_with_no_end_is_cut(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = modelled(world, "");
	voe_3d_model_intent intent = { .entity = entity };

	memset(intent.model.path, 'a', VOE_3D_MODEL_PATH);
	VOE_TEST_CHECK(voe_3d_model_submit(world, intent));
	voe_3d_model_system_run(world);
	VOE_TEST_CHECK_INT(strlen(voe_3d_model_get(world, entity)->path),
			   VOE_3D_MODEL_PATH - 1);
	// A run that corrects nothing ends the report's run.
	voe_3d_model_system_run(world);
	voe_base_arena_clear(arena);
}

static void a_material_with_no_end_is_cut(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = modelled(world, "Assets/hull.glb");
	voe_3d_model_intent intent = { .entity = entity,
				       .model = { .path = "Assets/hull.glb" } };

	memset(intent.model.materials[3], 'm', VOE_3D_MODEL_PATH);
	VOE_TEST_CHECK(voe_3d_model_submit(world, intent));
	voe_3d_model_system_run(world);
	VOE_TEST_CHECK_INT(strlen(voe_3d_model_get(world, entity)->materials[3]),
			   VOE_3D_MODEL_PATH - 1);
	VOE_TEST_CHECK(voe_3d_model_get(world, entity)->materials[4][0] == '\0');
	VOE_TEST_CHECK(strcmp(voe_3d_model_get(world, entity)->path,
			      "Assets/hull.glb") == 0);
	voe_3d_model_system_run(world);
	voe_base_arena_clear(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	the_description_is_a_path_cast_shadows_and_fade();
	the_default_casts_and_an_intent_turns_it_off(arena);
	a_submitted_fade_reads_back(arena);
	a_submitted_path_reads_back(arena);
	a_dead_entitys_intent_is_dropped(arena);
	a_path_with_no_end_is_cut(arena);
	a_material_with_no_end_is_cut(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
