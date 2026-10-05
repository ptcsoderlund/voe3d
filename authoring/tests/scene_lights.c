// Two directional lights through a save (0349): a sun and a moon, each with its
// own transform and light, are written by voe_authoring_scene_write and read
// into a fresh world by voe_authoring_scene_read, and come back as two light
// rows, each with every field it was written with, cast_shadows included, and
// its own rotation. A file holding one light section with no cast_shadows, as a
// scene saved before 049, reads one light that casts (0324).
//
// THE WRITER AND READER ARE GENERIC: they walk each type's description and know
// nothing of lights. So this test guards the light's description, that every
// field is described and that two rows stay apart, not code of its own.
//
// The world is set up as scene_read_unsaid.c's is, with scene's real transform,
// identity and light registered. The old file's missing fields each print a
// warning to stderr; that is the report doing its job, not a failure.
#include <authoring/scene_read.h>
#include <authoring/scene_write.h>

#include <base/arena.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdint.h>
#include <string.h>

#define ENTITIES 8

static const voe_scene_light sun_light = {
	.colour = { 1.0f, 1.0f, 1.0f },
	.intensity = 3.0f,
	.fill_colour = { 0.5f, 0.6f, 0.8f },
	.fill_intensity = 0.25f,
	.bounces = 1,
	.cast_shadows = true,
	.bounce_strength = 1.0f,
};

static const voe_scene_light moon_light = {
	.colour = { 0.6f, 0.7f, 1.0f },
	.intensity = 0.2f,
	.bounces = 0,
	.cast_shadows = false,
	.bounce_strength = 1.0f,
};

static const voe_math_quat sun_turn = { -0.38268343f, 0.0f, 0.0f, 0.9238795f };
static const voe_math_quat moon_turn = { 0.0f, 0.70710677f, -0.5f, 0.5f };

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_world *world = voe_ecs_world_new(arena, (voe_ecs_limits){
		.entities = ENTITIES,
		.component_types = 8,
		.intent_types = 8,
	});

	voe_scene_transform_register(world, ENTITIES);
	voe_scene_identity_register(world, ENTITIES);
	voe_scene_light_register(world, ENTITIES);
	return world;
}

// An entity with authored id `id`, turned by `turn` and lit by `light`.
static void add_light(voe_ecs_world *world, uint64_t id, const char *name,
		      voe_math_quat turn, voe_scene_light light)
{
	voe_ecs_entity entity = { 0 };
	voe_scene_identity identity = { .id = id };

	memcpy(identity.name, name, strlen(name));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_identity_add(world, entity, identity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, (voe_scene_transform){
		.rotation = turn,
		.scale = { 1.0f, 1.0f, 1.0f },
	}));
	VOE_TEST_CHECK(voe_scene_light_add(world, entity, light));
}

// The entity with authored id `id`, or the zero entity.
static voe_ecs_entity entity_of(const voe_ecs_world *world, uint64_t id)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(world);
	const voe_ecs_entity *entities = voe_scene_identity_entities(world);

	for (uint32_t i = 0; i < voe_scene_identity_count(world); i++)
		if (rows[i].id == id)
			return entities[i];
	return (voe_ecs_entity){ 0 };
}

static bool same_float3(voe_math_float3 a, voe_math_float3 b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

static bool same_light(const voe_scene_light *a, const voe_scene_light *b)
{
	return same_float3(a->colour, b->colour) && a->intensity == b->intensity &&
	       same_float3(a->fill_colour, b->fill_colour) &&
	       a->fill_intensity == b->fill_intensity &&
	       a->bounces == b->bounces && a->cast_shadows == b->cast_shadows &&
	       a->bounce_strength == b->bounce_strength;
}

static bool same_quat(voe_math_quat a, voe_math_quat b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

// The entity `id` in `world` has `light` and is turned by `turn`.
static void check_light(const voe_ecs_world *world, uint64_t id,
			const voe_scene_light *light, voe_math_quat turn)
{
	voe_ecs_entity entity = entity_of(world, id);
	const voe_scene_light *row = voe_scene_light_get(world, entity);
	const voe_scene_transform *place = voe_scene_transform_get(world, entity);

	VOE_TEST_CHECK(row != NULL && place != NULL);
	if (row != NULL && place != NULL) {
		VOE_TEST_CHECK(same_light(row, light));
		VOE_TEST_CHECK(same_quat(place->rotation, turn));
	}
}

static void test_two_lights_round_trip(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	voe_ecs_world *first = world_of(arena);
	voe_authoring_text out = { 0 };

	add_light(first, 1, "Sun", sun_turn, sun_light);
	add_light(first, 2, "Moon", moon_turn, moon_light);
	VOE_TEST_CHECK(voe_authoring_scene_write(first, NULL, arena, &out));

	voe_ecs_world *second = world_of(arena);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(out.text != NULL &&
		       voe_authoring_scene_read(out.text, out.size, second, arena,
						&kept));
	VOE_TEST_CHECK_INT(voe_scene_light_count(second), 2);
	check_light(second, 1, &sun_light, sun_turn);
	check_light(second, 2, &moon_light, moon_turn);
	voe_base_arena_destroy(arena);
}

// A light section as a scene saved before 049 wrote it: no cast_shadows line.
static void test_old_light_casts(void)
{
	static const char text[] = "[1]\n"
				   "name = \"Old sun\"\n"
				   "folded = false\n"
				   "[1.voe_scene_light]\n"
				   "colour = [1, 1, 1]\n"
				   "intensity = 2\n";
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(voe_authoring_scene_read(text, strlen(text), world, arena,
						&kept));
	VOE_TEST_CHECK_INT(voe_scene_light_count(world), 1);
	if (voe_scene_light_count(world) == 1)
		VOE_TEST_CHECK(voe_scene_light_rows(world)[0].cast_shadows);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_two_lights_round_trip();
	test_old_light_casts();
	return voe_test_result();
}
