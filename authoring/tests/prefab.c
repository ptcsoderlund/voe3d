// The prefab writer: that one tree comes out as the exact bytes of a `.prefab`
// file (ADR-0283 point 1) — the tree and nothing else, the root at the origin
// with no parent section — and that a root that cannot head a prefab is refused.
//
// THE EXPECTATION IS A STRING LITERAL COMPARED BYTE FOR BYTE, as in
// scene_write.c's test, so order and blank lines are checked with the rest.
//
// Refusals print a line to stderr; that is the report doing its job.
#include <authoring/prefab.h>

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/parent_component.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ENTITIES 16

#define LINK_FIELDS(F, F_READ_ONLY) F(voe_ecs_entity, target, ENTITY)

VOE_BASE_DESCRIBE_STRUCT(link, LINK_FIELDS)

static const struct voe_ecs_key link_key = { "test_link" };

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_world *world = voe_ecs_world_new(arena, (voe_ecs_limits){
		.entities = ENTITIES,
		.component_types = 8,
		.intent_types = 8,
	});

	voe_scene_transform_register(world, ENTITIES);
	voe_scene_identity_register(world, ENTITIES);
	voe_scene_parent_register(world, ENTITIES);
	voe_ecs_component_register(world, &link_key, sizeof(link), ENTITIES,
				   link_description());
	return world;
}

static voe_ecs_entity entity_of(voe_ecs_world *world)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	return entity;
}

// A transform at `position` with no turn and scale one.
static voe_scene_transform at(double x, double y, double z)
{
	return (voe_scene_transform){
		.position = { x, y, z },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
}

// An authored thing with `transform`, under `parent` unless it is NULL.
static voe_ecs_entity placed(voe_ecs_world *world, uint64_t id,
			     const char *name, voe_scene_transform transform,
			     const voe_ecs_entity *parent)
{
	voe_ecs_entity entity = entity_of(world);
	voe_scene_identity identity = { .id = id };

	memcpy(identity.name, name, strlen(name));
	VOE_TEST_CHECK(voe_scene_identity_add(world, entity, identity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, transform));
	if (parent != NULL)
		VOE_TEST_CHECK(voe_ecs_component_add(
			world, voe_ecs_component_type(world, &voe_scene_parent_key),
			entity, &(voe_scene_parent){ .parent = *parent }));
	return entity;
}

static void add_link(voe_ecs_world *world, voe_ecs_entity from,
		     voe_ecs_entity to)
{
	VOE_TEST_CHECK(voe_ecs_component_add(
		world, voe_ecs_component_type(world, &link_key), from,
		&(link){ .target = to }));
}

static void check_text(const char *actual, size_t size, const char *expected)
{
	if (size == strlen(expected) && memcmp(actual, expected, size) == 0 &&
	    actual[size] == '\0')
		return;

	voe_test_report(__FILE__, __LINE__, "the prefab text == the expected text");
	fprintf(stderr, "      actual (%zu bytes):\n%.*s\n      expected:\n%s\n",
		size, (int)size, actual, expected);
}

static void test_tree(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_scene_transform turned = at(3.0, 0.0, 5.0);

	// The hull turned, scaled and parented in the world; the file takes none.
	turned.rotation = (voe_math_quat){ 0.0f, 0.70710677f, 0.0f, 0.70710677f };
	turned.scale = (voe_math_float3){ 2.0f, 2.0f, 2.0f };

	voe_ecs_entity ground = placed(world, 5, "Ground", at(0, 0, 0), NULL);
	voe_ecs_entity hull = placed(world, 1, "Hull", turned, &ground);
	voe_ecs_entity turret = placed(world, 2, "Turret", at(0, 1, 0), &hull);
	voe_ecs_entity barrel = placed(world, 3, "Barrel", at(0, 0, 2), &turret);
	voe_ecs_entity other = placed(world, 4, "Other", at(0, 0, 0), NULL);
	voe_authoring_text out = { 0 };

	add_link(world, barrel, other);
	add_link(world, other, turret);

	VOE_TEST_CHECK(voe_authoring_prefab_write(world, hull, arena, &out));
	check_text(out.text != NULL ? out.text : "", out.size,
		   "[1]\n"
		   "name = \"Hull\"\n"
		   "[1.voe_scene_transform]\n"
		   "position = [0, 0, 0]\n"
		   "rotation = [0, 0, 0, 1]\n"
		   "scale = [1, 1, 1]\n"
		   "\n"
		   "[2]\n"
		   "name = \"Turret\"\n"
		   "[2.voe_scene_parent]\n"
		   "parent = 1\n"
		   "[2.voe_scene_transform]\n"
		   "position = [0, 1, 0]\n"
		   "rotation = [0, 0, 0, 1]\n"
		   "scale = [1, 1, 1]\n"
		   "\n"
		   "[3]\n"
		   "name = \"Barrel\"\n"
		   "[3.test_link]\n"
		   "target = 0\n"
		   "[3.voe_scene_parent]\n"
		   "parent = 2\n"
		   "[3.voe_scene_transform]\n"
		   "position = [0, 0, 2]\n"
		   "rotation = [0, 0, 0, 1]\n"
		   "scale = [1, 1, 1]\n");

	voe_base_arena_destroy(arena);
}

static void check_refused(const voe_ecs_world *world, voe_ecs_entity root,
			  voe_base_arena *arena)
{
	const char *sentinel = "untouched";
	voe_authoring_text out = { .text = sentinel, .size = 12345 };

	VOE_TEST_CHECK(!voe_authoring_prefab_write(world, root, arena, &out));
	VOE_TEST_CHECK(out.text == sentinel);
	VOE_TEST_CHECK_INT(out.size, 12345);
}

static void test_refusals(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity dead = placed(world, 1, "Dead", at(0, 0, 0), NULL);

	voe_ecs_entity_destroy(world, dead);
	// A dead root, and a live one with no identity.
	check_refused(world, dead, arena);
	check_refused(world, entity_of(world), arena);

	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_tree();
	test_refusals();
	return voe_test_result();
}
