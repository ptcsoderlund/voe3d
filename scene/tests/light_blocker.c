// The light blocker: what registration tells a tool (the menu path, the
// transform it needs, the default size of one metre each way), that a row
// arrives as given, that a replace lands only when the system runs, that a
// negative, an infinite and a NaN size are each refused and keep the row, and
// that adding to a dead entity is false.
//
// EACH REFUSAL IS ITS OWN INTENT, submitted one at a time with the row checked
// after each, so a refusal that let one through shows as that one.
//
// THE DESCRIPTION IS SWITCHED ON HERE, as scene/tests/light.c does and for the
// reason scene/tests/transform.c gives, so the field list is checked in every
// build.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <math/float3.h>
#include <scene/light_blocker_component.h>
#include <scene/light_blocker_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <string.h>

#define BLOCKERS 4

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = BLOCKERS,
		.component_types = 2,
		.intent_types = 2,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, BLOCKERS);
	voe_scene_light_blocker_register(world, BLOCKERS);
	return world;
}

// No component one, so a default copied in shows as a wrong number.
static voe_scene_light_blocker known(void)
{
	return (voe_scene_light_blocker){ .size = { 8.0f, 3.0f, 6.5f } };
}

static void check_blocker(const voe_scene_light_blocker *read,
			  voe_scene_light_blocker expected)
{
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(read->size.x, expected.size.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->size.y, expected.size.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->size.z, expected.size.z, 0.0f);
}

static void registration_says_what_a_blocker_is(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type =
		voe_ecs_component_type(world, &voe_scene_light_blocker_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);
	voe_ecs_type needed = { 0 };
	const voe_base_struct_description *description =
		voe_scene_light_blocker_description();

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT(
		(long long)replace.row_offset,
		(long long)offsetof(voe_scene_light_blocker_intent, blocker));
	check_blocker(voe_ecs_component_default(world, type),
		      (voe_scene_light_blocker){ .size = { 1.0f, 1.0f, 1.0f } });
	VOE_TEST_CHECK(voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK(voe_ecs_component_key(world, needed) ==
		       &voe_scene_transform_key);
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Rendering / Light blocker") == 0);
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, type));

	VOE_TEST_CHECK_INT(description->field_count, 1);
	VOE_TEST_CHECK(strcmp(description->fields[0].name, "size") == 0);
	VOE_TEST_CHECK_INT(description->fields[0].kind, VOE_BASE_FIELD_FLOAT3);
}

static void a_blocker_arrives_as_given(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity house = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &house));
	VOE_TEST_CHECK(voe_scene_light_blocker_get(world, house) == NULL);
	VOE_TEST_CHECK(voe_scene_light_blocker_add(world, house, known()));

	check_blocker(voe_scene_light_blocker_get(world, house), known());
	VOE_TEST_CHECK_INT(voe_scene_light_blocker_count(world), 1);
	VOE_TEST_CHECK(voe_scene_light_blocker_rows(world) ==
		       voe_scene_light_blocker_get(world, house));
	VOE_TEST_CHECK_INT(voe_scene_light_blocker_entities(world)[0].index,
			   house.index);
}

static void a_replace_lands_when_the_system_runs(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity house = { 0 };
	voe_scene_light_blocker bigger = { .size = { 10.0f, 4.0f, 0.0f } };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &house));
	VOE_TEST_CHECK(voe_scene_light_blocker_add(world, house, known()));
	VOE_TEST_CHECK(voe_scene_light_blocker_submit(
		world, (voe_scene_light_blocker_intent){ .entity = house,
							 .blocker = bigger }));
	check_blocker(voe_scene_light_blocker_get(world, house), known());
	voe_scene_light_blocker_system_run(world);
	check_blocker(voe_scene_light_blocker_get(world, house), bigger);
}

static void a_refused_size_keeps_the_row(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity house = { 0 };
	voe_scene_light_blocker bad[3] = { known(), known(), known() };

	bad[0].size.x = -1.0f;
	bad[1].size.y = INFINITY;
	bad[2].size.z = NAN;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &house));
	VOE_TEST_CHECK(voe_scene_light_blocker_add(world, house, known()));

	for (int i = 0; i < 3; i++) {
		VOE_TEST_CHECK(voe_scene_light_blocker_submit(
			world, (voe_scene_light_blocker_intent){
				       .entity = house, .blocker = bad[i] }));
		voe_scene_light_blocker_system_run(world);
		check_blocker(voe_scene_light_blocker_get(world, house),
			      known());
	}
}

static void adding_to_a_dead_entity_is_false(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity gone = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &gone));
	voe_ecs_entity_destroy(world, gone);
	VOE_TEST_CHECK(!voe_scene_light_blocker_add(world, gone, known()));
	VOE_TEST_CHECK_INT(voe_scene_light_blocker_count(world), 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	registration_says_what_a_blocker_is(arena);
	a_blocker_arrives_as_given(arena);
	a_replace_lands_when_the_system_runs(arena);
	a_refused_size_keeps_the_row(arena);
	adding_to_a_dead_entity_is_false(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
