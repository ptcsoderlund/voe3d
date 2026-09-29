// The prefab writer: that one tree comes out as the exact bytes of a `.prefab`
// file (ADR-0283 point 1) — the tree and nothing else, the root at the origin
// with no parent section — and that a root that cannot head a prefab is refused.
// The prefab reader: that text read back onto a placed root (0283 point 4), and
// a text that is not one tree, or holds a camera or prefab, creating nothing.
// The prefab cook: that text read into a world and cooked into a spawning
// function (0283 point 9), and two roots refused.
//
// THE EXPECTATION IS A STRING LITERAL COMPARED BYTE FOR BYTE, as in
// scene_write.c's test, so order and blank lines are checked with the rest.
//
// Refusals print a line to stderr; that is the report doing its job.
#include <authoring/prefab.h>
#include <authoring/scene_read.h>

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/parent_component.h>
#include <scene/parent_system.h>
#include <scene/prefab_component.h>
#include <scene/prefab_system.h>
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

#define TEST_SHAPE_FIELDS(F, F_READ_ONLY) F(uint32_t, kind, UINT32)

VOE_BASE_DESCRIBE_STRUCT(test_shape, TEST_SHAPE_FIELDS)

static const struct voe_ecs_key test_shape_key = { "test_shape" };

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
	voe_scene_prefab_register(world, ENTITIES);
	voe_ecs_component_register(world, &link_key, sizeof(link), ENTITIES,
				   link_description());
	voe_ecs_component_register(world, &test_shape_key, sizeof(test_shape),
				   ENTITIES, test_shape_description());
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

static bool same(voe_ecs_entity a, voe_ecs_entity b)
{
	return a.index == b.index && a.generation == b.generation;
}

// The entity whose identity has `id`, checked to exist.
static voe_ecs_entity by_id(const voe_ecs_world *world, uint64_t id)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(world);

	for (uint32_t i = 0; i < voe_scene_identity_count(world); i++)
		if (rows[i].id == id)
			return voe_scene_identity_entities(world)[i];
	VOE_TEST_CHECK(!"an identity with the id");
	return (voe_ecs_entity){ 0 };
}

static void check_part(const voe_ecs_world *world, voe_ecs_entity entity,
		       voe_ecs_entity root)
{
	const voe_scene_prefab_part *part =
		voe_scene_prefab_part_get(world, entity);

	VOE_TEST_CHECK(part != NULL && same(part->instance, root));
}

static void test_read(void)
{
	voe_base_arena *arena = voe_base_arena_new(128 * 1024);
	voe_ecs_world *source = world_of(arena);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity hull = placed(source, 1, "Hull", at(0, 0, 0), NULL);
	voe_ecs_entity turret = placed(source, 2, "Turret", at(0, 1, 0), &hull);
	voe_authoring_text text = { 0 };
	uint64_t next = 0;

	(void)placed(source, 3, "Barrel", at(0, 0, 2), &turret);
	add_link(source, hull, turret);
	VOE_TEST_CHECK(voe_authoring_prefab_write(source, hull, arena, &text));

	// A fresh root, as placing a prefab makes one.
	voe_ecs_entity root = placed(world, 7, "Tank", at(3, 0, 5), NULL);

	VOE_TEST_CHECK(voe_authoring_prefab_read(text.text, text.size, world,
						 root, 100, arena, &next));
	VOE_TEST_CHECK_INT(next, 102);

	voe_ecs_entity made_turret = by_id(world, 100);
	voe_ecs_entity made_barrel = by_id(world, 101);
	const voe_scene_transform *kept = voe_scene_transform_get(world, root);
	const link *hull_link = voe_ecs_component_get(
		world, voe_ecs_component_type(world, &link_key), root);

	// The root keeps its identity and place and gains the hull's row.
	VOE_TEST_CHECK(same(by_id(world, 7), root));
	VOE_TEST_CHECK(kept->position.x == 3.0 && kept->position.z == 5.0);
	VOE_TEST_CHECK(voe_scene_parent_get(world, root) == NULL);
	VOE_TEST_CHECK(hull_link != NULL && same(hull_link->target, made_turret));

	VOE_TEST_CHECK(strcmp(voe_scene_identity_get(world, made_turret)->name,
			      "Turret") == 0);
	VOE_TEST_CHECK(strcmp(voe_scene_identity_get(world, made_barrel)->name,
			      "Barrel") == 0);
	VOE_TEST_CHECK(same(voe_scene_parent_get(world, made_turret)->parent, root));
	VOE_TEST_CHECK(same(voe_scene_parent_get(world, made_barrel)->parent,
			    made_turret));
	check_part(world, root, root);
	check_part(world, made_turret, root);
	check_part(world, made_barrel, root);

	voe_base_arena_destroy(arena);
}

static void check_read_refused(const char *text)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity root = placed(world, 7, "Tank", at(0, 0, 0), NULL);
	uint32_t entities = voe_ecs_entity_count(world);
	uint64_t next = 12345;

	VOE_TEST_CHECK(!voe_authoring_prefab_read(text, strlen(text), world,
						  root, 100, arena, &next));
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), entities);
	VOE_TEST_CHECK_INT(next, 12345);
	VOE_TEST_CHECK(voe_scene_prefab_part_get(world, root) == NULL);

	voe_base_arena_destroy(arena);
}

static void test_read_refusals(void)
{
	check_read_refused("[1]\nname = \"A\"\n[2]\nname = \"B\"\n");
	check_read_refused("[1]\nname = \"A\"\n[1.voe_scene_camera]\nfov = 1\n");
	check_read_refused("[1]\nname = \"A\"\n"
			   "[1.voe_scene_prefab]\npath = \"Assets/a.prefab\"\n");
}

static bool holds(const voe_authoring_text *text, const char *part)
{
	return text->text != NULL && strstr(text->text, part) != NULL;
}

static void test_cook(void)
{
	voe_base_arena *arena = voe_base_arena_new(128 * 1024);
	voe_ecs_world *source = world_of(arena);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity hull = placed(source, 1, "Hull", at(0, 0, 0), NULL);
	voe_authoring_text text = { 0 };
	voe_authoring_text cooked = { 0 };
	voe_authoring_kept kept = { 0 };
	uint32_t entities = 0;

	(void)placed(source, 2, "Turret", at(0, 1, 0), &hull);
	VOE_TEST_CHECK(voe_ecs_component_add(
		source, voe_ecs_component_type(source, &test_shape_key), hull,
		&(test_shape){ .kind = 3 }));
	VOE_TEST_CHECK(voe_authoring_prefab_write(source, hull, arena, &text));
	VOE_TEST_CHECK(voe_authoring_scene_read(text.text, text.size, world,
						arena, &kept));

	VOE_TEST_CHECK(voe_authoring_prefab_cook(world, "enemy_tank", arena,
						 &cooked, &entities));
	VOE_TEST_CHECK_INT(entities, 2);
	VOE_TEST_CHECK(holds(&cooked, "static bool enemy_tank("));
	VOE_TEST_CHECK(holds(&cooked, "voe_ecs_structure_add(world, "
				      "voe_ecs_component_type(world, "
				      "&test_shape_key), entities[0], "
				      "&(test_shape){ .kind = 3u }))"));
	VOE_TEST_CHECK(holds(&cooked, "entities[0], &(voe_scene_transform){ "
				      ".position = position, .rotation = "
				      "rotation, .scale = { 0x1p+0f, 0x1p+0f, "
				      "0x1p+0f } }"));
	VOE_TEST_CHECK(holds(&cooked, "entities[1], &(voe_scene_parent){ "
				      ".parent = entities[0] }"));
	VOE_TEST_CHECK(holds(&cooked, "entities[1], &(voe_scene_transform){ "));
	VOE_TEST_CHECK(!holds(&cooked, "voe_scene_identity"));
	VOE_TEST_CHECK(!holds(&cooked, "#include"));

	// Two roots.
	voe_ecs_world *two = world_of(arena);
	voe_authoring_text sentinel = { .text = "untouched", .size = 9 };

	(void)placed(two, 1, "A", at(0, 0, 0), NULL);
	(void)placed(two, 2, "B", at(0, 0, 0), NULL);
	entities = 12345;
	VOE_TEST_CHECK(!voe_authoring_prefab_cook(two, "enemy_tank", arena,
						  &sentinel, &entities));
	VOE_TEST_CHECK(strcmp(sentinel.text, "untouched") == 0);
	VOE_TEST_CHECK_INT(entities, 12345);

	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_tree();
	test_refusals();
	test_read();
	test_read_refusals();
	test_cook();
	return voe_test_result();
}
