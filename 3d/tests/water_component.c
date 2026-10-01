// The water component: that both tables register, the waves runtime-only, that
// an added row reads back, that the default row is 0305's, that the description
// names every field with its kind, and that a replace is queued. Needs no
// graphics card.
//
// THE DESCRIPTION IS SWITCHED ON HERE, WHATEVER THE BUILD SAID, for the reason
// scene/tests/identity.c gives: check.cmake builds without descriptions.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <3d/water_component.h>

#include <base/arena.h>

#include <ecs/component.h>
#include <ecs/intent.h>
#include <ecs/world.h>

#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <string.h>

#define SCRATCH (64 * 1024)
#define ENTITIES 4

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = ENTITIES,
		.component_types = 3,
		.intent_types = 2,
		.structure_requests = 4 * ENTITIES,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	// Transform first: water's registration names what it needs.
	voe_scene_transform_register(world, ENTITIES);
	voe_3d_water_register(world, ENTITIES);
	return world;
}

static void both_tables_register(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = { 0 };
	const voe_3d_water *row;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_3d_water_add(
		world, entity, (voe_3d_water){ .width = 5.0f, .deep = 1.0f }));
	row = voe_3d_water_get(world, entity);
	VOE_TEST_CHECK(row != NULL && row->width == 5.0f && row->deep == 1.0f);
	VOE_TEST_CHECK_INT(voe_3d_water_count(world), 1);
	VOE_TEST_CHECK(voe_3d_water_rows(world)[0].width == 5.0f);
	VOE_TEST_CHECK(voe_3d_water_entities(world)[0].index == entity.index);
	VOE_TEST_CHECK_INT(voe_3d_waves_count(world), 0);
	VOE_TEST_CHECK(voe_3d_waves_get(world, entity) == NULL);
	VOE_TEST_CHECK(voe_ecs_component_runtime_only(
		world, voe_ecs_component_type(world, &voe_3d_waves_key)));
	voe_base_arena_clear(arena);
}

static bool near(float a, float b)
{
	return fabsf(a - b) < 1e-6f;
}

static void the_default_row_is_0305s(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_type type = voe_ecs_component_type(world, &voe_3d_water_key);
	const voe_3d_water *row = voe_ecs_component_default(world, type);

	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	VOE_TEST_CHECK(row->width == 20.0f && row->length == 20.0f);
	VOE_TEST_CHECK(near(row->wave_height, 0.05f) && row->wave_length == 2);
	VOE_TEST_CHECK(row->deep == 3.0f);
	VOE_TEST_CHECK(near(row->colour.x, 0.02f) && near(row->colour.y, 0.09f) &&
		       near(row->colour.z, 0.10f));
	VOE_TEST_CHECK(near(row->sky.x, 0.55f) && near(row->sky.y, 0.70f) &&
		       near(row->sky.z, 0.85f));
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Rendering / Water") == 0);
	voe_base_arena_clear(arena);
}

static void the_description_names_every_field(void)
{
	static const struct {
		const char *name;
		voe_base_field_kind kind;
	} want[] = {
		{ "width", VOE_BASE_FIELD_FLOAT32 },
		{ "length", VOE_BASE_FIELD_FLOAT32 },
		{ "wave_height", VOE_BASE_FIELD_FLOAT32 },
		{ "wave_length", VOE_BASE_FIELD_FLOAT32 },
		{ "deep", VOE_BASE_FIELD_FLOAT32 },
		{ "colour", VOE_BASE_FIELD_COLOUR },
		{ "sky", VOE_BASE_FIELD_COLOUR },
	};
	const voe_base_struct_description *description =
		voe_3d_water_description();
	const uint32_t count = sizeof(want) / sizeof(want[0]);

	VOE_TEST_CHECK(strcmp(description->name, "voe_3d_water") == 0);
	VOE_TEST_CHECK_INT(description->field_count, count);
	if (description->field_count != count)
		return;
	for (uint32_t i = 0; i < count; i++) {
		VOE_TEST_CHECK(strcmp(description->fields[i].name,
				      want[i].name) == 0);
		VOE_TEST_CHECK_INT(description->fields[i].kind, want[i].kind);
	}
}

static void a_replace_is_queued(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = { 0 };
	voe_ecs_intent replace = voe_ecs_component_replace(
		world, voe_ecs_component_type(world, &voe_3d_water_key)).intent;
	const voe_3d_water_intent *queued;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_3d_water_submit(
		world, (voe_3d_water_intent){ .entity = entity,
					      .water = { .deep = 7.0f } }));
	VOE_TEST_CHECK_INT(voe_ecs_intent_count(world, replace), 1);
	queued = voe_ecs_intent_queue(world, replace);
	VOE_TEST_CHECK(queued != NULL && queued[0].water.deep == 7.0f);
	voe_base_arena_clear(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	both_tables_register(arena);
	the_default_row_is_0305s(arena);
	the_description_names_every_field();
	a_replace_is_queued(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
