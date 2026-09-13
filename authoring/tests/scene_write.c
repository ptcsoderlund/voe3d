// The scene writer: that a world comes out as the exact bytes ADR-0149 names,
// the same bytes every time, and that each value a file could not carry back is
// refused rather than written.
//
// EVERY EXPECTATION IS A STRING LITERAL COMPARED BYTE FOR BYTE. Order, blank
// lines and the final newline are what make two saves of one scene diff clean,
// and a check that parsed the output would forgive exactly those.
//
// THE EVERY-KIND COMPONENT'S DESCRIPTION IS WRITTEN BY HAND, NOT WITH
// VOE_BASE_DESCRIBE_STRUCT, because describe.h lets only ENUM, CHAR and ENTITY
// repeat and the writer's array nesting — `[[0, 0, 0], [1, 1, 1]]` — still has
// to be proven for a FLOAT3 array. The world never reads a description, so a
// table written by hand is as good as one the macro wrote.
//
// Refusals and warnings print a line to stderr; that is the report doing its job,
// not a failure.
#include <authoring/scene_write.h>

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTITIES 16

static void check_text(const char *actual, size_t size, const char *expected,
		       const char *file, int line)
{
	if (size == strlen(expected) && memcmp(actual, expected, size) == 0 &&
	    actual[size] == '\0')
		return;

	voe_test_report(file, line, "the written text == the expected text");
	fprintf(stderr, "      actual (%zu bytes):\n%.*s\n      expected (%zu bytes):\n%s\n",
		size, (int)size, actual, strlen(expected), expected);
}

#define CHECK_TEXT(actual, size, expected) \
	check_text((actual), (size), (expected), __FILE__, __LINE__)

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_world *world = voe_ecs_world_new(arena, (voe_ecs_limits){
		.entities = ENTITIES,
		.component_types = 8,
		.intent_types = 8,
	});

	voe_scene_transform_register(world, ENTITIES);
	voe_scene_identity_register(world, ENTITIES);
	return world;
}

static voe_ecs_entity entity_of(voe_ecs_world *world)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	return entity;
}

static voe_ecs_entity authored(voe_ecs_world *world, uint64_t id,
			       const char *name)
{
	voe_ecs_entity entity = entity_of(world);
	voe_scene_identity identity = { .id = id };

	VOE_TEST_CHECK(strlen(name) < VOE_SCENE_IDENTITY_NAME);
	memcpy(identity.name, name, strlen(name));
	VOE_TEST_CHECK(voe_scene_identity_add(world, entity, identity));
	return entity;
}

// Writes, checks it succeeded, and hands back the text.
static const char *written(const voe_ecs_world *world, voe_base_arena *arena,
			   size_t *size)
{
	const char *text = NULL;

	*size = 0;
	VOE_TEST_CHECK(voe_authoring_scene_write(world, NULL, arena, &text, size));
	return text != NULL ? text : "";
}

// Writes, checks it was refused, and that nothing was handed back.
static void check_refused(const voe_ecs_world *world, voe_base_arena *arena)
{
	const char *sentinel = "untouched";
	const char *text = sentinel;
	size_t size = 12345;

	VOE_TEST_CHECK(!voe_authoring_scene_write(world, NULL, arena, &text, &size));
	VOE_TEST_CHECK(text == sentinel);
	VOE_TEST_CHECK_INT(size, 12345);
}

static void test_three_entities(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity cube = authored(world, 1, "Cube");
	voe_ecs_entity probe = entity_of(world);
	size_t size;

	authored(world, 7, "Sun");
	VOE_TEST_CHECK(voe_scene_transform_add(world, cube, (voe_scene_transform){
		.position = { 1.0f, 2.5f, -3.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	}));
	voe_ecs_entity floor = authored(world, 3, "Floor \"big\"");

	VOE_TEST_CHECK(voe_scene_transform_add(world, floor, (voe_scene_transform){
		.position = { 0.0f, -0.5f, 0.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 10.0f, 0.1f, 10.0f },
	}));
	VOE_TEST_CHECK(voe_scene_transform_add(world, probe, (voe_scene_transform){
		.position = { 9.0f, 9.0f, 9.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	}));

	const char *text = written(world, arena, &size);

	CHECK_TEXT(text, size,
		   "[1]\n"
		   "name = \"Cube\"\n"
		   "[1.voe_scene_transform]\n"
		   "position = [1, 2.5, -3]\n"
		   "rotation = [0, 0, 0, 1]\n"
		   "scale = [1, 1, 1]\n"
		   "\n"
		   "[3]\n"
		   "name = \"Floor \\\"big\\\"\"\n"
		   "[3.voe_scene_transform]\n"
		   "position = [0, -0.5, 0]\n"
		   "rotation = [0, 0, 0, 1]\n"
		   "scale = [10, 0.1, 10]\n"
		   "\n"
		   "[7]\n"
		   "name = \"Sun\"\n");

	voe_base_arena_destroy(arena);
}

typedef struct {
	int8_t i8;
	int16_t i16;
	int32_t i32;
	int64_t i64;
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
	float f32;
	double f64;
	bool flag;
	float f2[2];
	float f3[3];
	float f4[4];
	float q[4];
	float m[16];
	float points[2][3];
	char label[16];
	voe_ecs_entity target;
	voe_ecs_entity pair[2];
} every;

#define ROW(field, KIND, elements)                                            \
	{                                                                     \
		.name = #field,                                               \
		.kind = VOE_BASE_FIELD_##KIND,                                \
		.offset = offsetof(every, field),                             \
		.size = sizeof(((every *)0)->field),                          \
		.count = (elements),                                          \
	}

static const voe_base_field_description every_rows[] = {
	ROW(i8, INT8, 1),        ROW(i16, INT16, 1),    ROW(i32, INT32, 1),
	ROW(i64, INT64, 1),      ROW(u8, UINT8, 1),     ROW(u16, UINT16, 1),
	ROW(u32, UINT32, 1),     ROW(u64, UINT64, 1),   ROW(f32, FLOAT32, 1),
	ROW(f64, FLOAT64, 1),    ROW(flag, BOOL, 1),    ROW(f2, FLOAT2, 1),
	ROW(f3, FLOAT3, 1),      ROW(f4, FLOAT4, 1),    ROW(q, QUAT, 1),
	ROW(m, FLOAT4X4, 1),     ROW(points, FLOAT3, 2), ROW(label, CHAR, 16),
	ROW(target, ENTITY, 1),  ROW(pair, ENTITY, 2),
};

static const voe_base_struct_description every_description = {
	.name = "every",
	.fields = every_rows,
	.field_count = sizeof(every_rows) / sizeof(every_rows[0]),
};

static const struct voe_ecs_key every_key = { "test_every" };
static const struct voe_ecs_key runtime_key = { "aaa_runtime" };

static void test_every_kind(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(
		world, &every_key, sizeof(every), ENTITIES, &every_description);
	voe_ecs_type runtime = voe_ecs_component_register(
		world, &runtime_key, sizeof(int), ENTITIES, &voe_ecs_runtime_only);
	voe_ecs_entity one = authored(world, 1, "Every");
	voe_ecs_entity two = authored(world, 2, "Other");
	voe_ecs_entity gone = entity_of(world);
	uint8_t two_as_bool = 2;
	int unsaved = 42;
	size_t size;

	voe_ecs_entity_destroy(world, gone);

	every row = {
		.i8 = -8,
		.i16 = -1600,
		.i32 = -320000,
		.i64 = -6400000000,
		.u8 = 200,
		.u16 = 60000,
		.u32 = 4000000000u,
		.u64 = UINT64_MAX,
		.f32 = 0.1f,
		.f64 = 0.1,
		.f2 = { 0.5f, -0.25f },
		.f3 = { 1.0f, 2.0f, 3.0f },
		.f4 = { 1.0f, 0.0f, 0.0f, 1.0f },
		.q = { 0.0f, 0.0f, 0.0f, 1.0f },
		.m = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
		.points = { { 0, 0, 0 }, { 1, 1, 1 } },
		.label = "say \"hi\" \\ ok",
		.target = one,
		.pair = { two, gone },
	};

	memcpy(&row.flag, &two_as_bool, 1);
	VOE_TEST_CHECK(voe_ecs_component_add(world, type, one, &row));
	VOE_TEST_CHECK(voe_ecs_component_add(world, runtime, one, &unsaved));

	const char *text = written(world, arena, &size);

	CHECK_TEXT(text, size,
		   "[1]\n"
		   "name = \"Every\"\n"
		   "[1.test_every]\n"
		   "i8 = -8\n"
		   "i16 = -1600\n"
		   "i32 = -320000\n"
		   "i64 = -6400000000\n"
		   "u8 = 200\n"
		   "u16 = 60000\n"
		   "u32 = 4000000000\n"
		   "u64 = 18446744073709551615\n"
		   "f32 = 0.1\n"
		   "f64 = 0.1\n"
		   "flag = true\n"
		   "f2 = [0.5, -0.25]\n"
		   "f3 = [1, 2, 3]\n"
		   "f4 = [1, 0, 0, 1]\n"
		   "q = [0, 0, 0, 1]\n"
		   "m = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]\n"
		   "points = [[0, 0, 0], [1, 1, 1]]\n"
		   "label = \"say \\\"hi\\\" \\\\ ok\"\n"
		   "target = 1\n"
		   "pair = [2, 0]\n"
		   "\n"
		   "[2]\n"
		   "name = \"Other\"\n");

	voe_base_arena_destroy(arena);
}

typedef struct {
	float f;
	double d;
} floats;

static const voe_base_field_description floats_rows[] = {
	{ .name = "f", .kind = VOE_BASE_FIELD_FLOAT32,
	  .offset = offsetof(floats, f), .size = sizeof(float), .count = 1 },
	{ .name = "d", .kind = VOE_BASE_FIELD_FLOAT64,
	  .offset = offsetof(floats, d), .size = sizeof(double), .count = 1 },
};

static const voe_base_struct_description floats_description = {
	.name = "floats",
	.fields = floats_rows,
	.field_count = 2,
};

static const struct voe_ecs_key floats_key = { "test_floats" };

static voe_ecs_world *floats_world(voe_base_arena *arena, floats value)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(
		world, &floats_key, sizeof(floats), ENTITIES, &floats_description);
	voe_ecs_entity entity = authored(world, 1, "F");

	VOE_TEST_CHECK(voe_ecs_component_add(world, type, entity, &value));
	return world;
}

// Writes f and d, checks the two lines are exactly the digits expected, and that
// those digits read back to the same bits.
static void check_floats(float f, double d, const char *f_digits,
			 const char *d_digits)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = floats_world(arena, (floats){ .f = f, .d = d });
	char expected[256];
	size_t size;

	(void)snprintf(expected, sizeof(expected),
		       "[1]\nname = \"F\"\n[1.test_floats]\nf = %s\nd = %s\n",
		       f_digits, d_digits);
	const char *text = written(world, arena, &size);

	CHECK_TEXT(text, size, expected);

	float f_back = strtof(f_digits, NULL);
	double d_back = strtod(d_digits, NULL);

	VOE_TEST_CHECK(memcmp(&f_back, &f, sizeof(f)) == 0);
	VOE_TEST_CHECK(memcmp(&d_back, &d, sizeof(d)) == 0);

	voe_base_arena_destroy(arena);
}

static void test_floats(void)
{
	check_floats(0.1f, 0.1, "0.1", "0.1");
	check_floats(1.0f, 1.0, "1", "1");
	check_floats(-0.0f, -0.0, "-0", "-0");
	check_floats(3.4028235e38f, 16777217.0, "3.4028235e+38", "16777217");
	check_floats(1.0f / 3.0f, 1.0 / 3.0, "0.33333334",
		     "0.3333333333333333");
	// %g alone would spell these `1e+01` and `1.2345679e+08`.
	check_floats(10.0f, 100.0, "10", "100");
	check_floats(123456792.0f, 1e-300, "123456792", "1e-300");
	check_floats(1e-5f, 0.001, "1e-05", "0.001");
}

static void test_ascending_ids(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	size_t size;

	authored(world, 3, "c");
	authored(world, 1, "a");
	authored(world, 2, "b");

	const char *text = written(world, arena, &size);

	CHECK_TEXT(text, size,
		   "[1]\nname = \"a\"\n\n[2]\nname = \"b\"\n\n[3]\nname = \"c\"\n");

	voe_base_arena_destroy(arena);
}

typedef struct {
	voe_ecs_entity target;
} link;

static const voe_base_field_description link_rows[] = {
	{ .name = "target", .kind = VOE_BASE_FIELD_ENTITY,
	  .offset = offsetof(link, target), .size = sizeof(voe_ecs_entity),
	  .count = 1 },
};

static const voe_base_struct_description link_description = {
	.name = "link",
	.fields = link_rows,
	.field_count = 1,
};

static const struct voe_ecs_key link_key = { "test_link" };

static void test_entity_without_identity(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(
		world, &link_key, sizeof(link), ENTITIES, &link_description);
	voe_ecs_entity source = authored(world, 4, "Source");
	link value = { .target = entity_of(world) };
	size_t size;

	VOE_TEST_CHECK(voe_ecs_component_add(world, type, source, &value));

	const char *text = written(world, arena, &size);

	CHECK_TEXT(text, size,
		   "[4]\nname = \"Source\"\n[4.test_link]\ntarget = 0\n");

	voe_base_arena_destroy(arena);
}

static void test_empty_world(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = voe_ecs_world_new(arena, (voe_ecs_limits){
		.entities = 4, .component_types = 4, .intent_types = 4 });
	size_t size;

	// Nothing registered, not even the identity.
	const char *text = written(world, arena, &size);

	CHECK_TEXT(text, size, "");

	// The identity registered and nothing authored.
	voe_scene_identity_register(world, 4);
	(void)entity_of(world);
	text = written(world, arena, &size);
	CHECK_TEXT(text, size, "");

	voe_base_arena_destroy(arena);
}

typedef enum {
	SHAPE_CUBE,
	SHAPE_SPHERE,
} shape;

typedef struct {
	shape kind;
} shaped;

static const voe_base_field_description shaped_rows[] = {
	{ .name = "kind", .kind = VOE_BASE_FIELD_ENUM,
	  .offset = offsetof(shaped, kind), .size = sizeof(shape), .count = 1 },
};

static const voe_base_struct_description shaped_description = {
	.name = "shaped",
	.fields = shaped_rows,
	.field_count = 1,
};

static const struct voe_ecs_key shaped_key = { "test_shaped" };

static void test_refusals(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	// An enum.
	{
		voe_ecs_world *world = world_of(arena);
		voe_ecs_type type = voe_ecs_component_register(
			world, &shaped_key, sizeof(shaped), ENTITIES,
			&shaped_description);
		voe_ecs_entity entity = authored(world, 1, "Shape");
		shaped value = { .kind = SHAPE_SPHERE };

		VOE_TEST_CHECK(voe_ecs_component_add(world, type, entity, &value));
		check_refused(world, arena);
	}

	// A NaN, and an infinity, in each float kind and in a vector.
	check_refused(floats_world(arena, (floats){ .f = NAN, .d = 1.0 }), arena);
	check_refused(floats_world(arena, (floats){ .f = 1.0f, .d = INFINITY }),
		      arena);
	{
		voe_ecs_world *world = world_of(arena);
		voe_ecs_entity entity = authored(world, 1, "Far");

		VOE_TEST_CHECK(voe_scene_transform_add(world, entity, (voe_scene_transform){
			.position = { 0.0f, -INFINITY, 0.0f },
			.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
			.scale = { 1.0f, 1.0f, 1.0f },
		}));
		check_refused(world, arena);
	}

	// A control character in a name.
	{
		voe_ecs_world *world = world_of(arena);

		authored(world, 1, "tab\there");
		check_refused(world, arena);
	}

	// Two entities with one id.
	{
		voe_ecs_world *world = world_of(arena);

		authored(world, 5, "one");
		authored(world, 2, "between");
		authored(world, 5, "two");
		check_refused(world, arena);
	}

	voe_base_arena_destroy(arena);
}

static void test_same_bytes_twice(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	size_t first_size;
	size_t second_size;

	for (uint64_t id = 9; id > 0; id--) {
		char name[8];
		voe_ecs_entity entity;

		(void)snprintf(name, sizeof(name), "e%u", (unsigned)id);
		entity = authored(world, id * 11 % 13, name);
		VOE_TEST_CHECK(voe_scene_transform_add(world, entity, (voe_scene_transform){
			.position = { (float)id / 7.0f, 0.0f, 0.0f },
			.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
			.scale = { 1.0f, 1.0f, 1.0f },
		}));
	}

	const char *first = written(world, arena, &first_size);
	const char *second = written(world, arena, &second_size);

	VOE_TEST_CHECK(first != second);
	VOE_TEST_CHECK_INT(first_size, second_size);
	VOE_TEST_CHECK(memcmp(first, second, first_size) == 0);

	voe_base_arena_destroy(arena);
}

static void test_kept_sections(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity cube = authored(world, 2, "Cube");
	size_t size;

	authored(world, 5, "Lamp");
	VOE_TEST_CHECK(voe_scene_transform_add(world, cube, (voe_scene_transform){
		.position = { 0.0f, 0.0f, 0.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	}));

	// Out of order, one on each side of voe_scene_transform, one on an entity
	// with no component sections, and two whose id no entity has.
	const voe_authoring_kept_section sections[] = {
		{ .id = 2, .key = "zz_after", .lines = "b = 2\n", .size = 6 },
		{ .id = 9, .key = "gone", .lines = "c = 3\n", .size = 6 },
		{ .id = 2, .key = "aa_before", .lines = "a = 1\nx = \"y\"\n",
		  .size = 14 },
		{ .id = 5, .key = "voe_game_glow", .lines = "", .size = 0 },
		{ .id = 1, .key = "gone", .lines = "d = 4\n", .size = 6 },
	};
	const voe_authoring_kept kept = { .sections = sections, .count = 5 };
	const char *text = NULL;

	VOE_TEST_CHECK(voe_authoring_scene_write(world, &kept, arena, &text, &size));
	CHECK_TEXT(text != NULL ? text : "", size,
		   "[2]\n"
		   "name = \"Cube\"\n"
		   "[2.aa_before]\n"
		   "a = 1\n"
		   "x = \"y\"\n"
		   "[2.voe_scene_transform]\n"
		   "position = [0, 0, 0]\n"
		   "rotation = [0, 0, 0, 1]\n"
		   "scale = [1, 1, 1]\n"
		   "[2.zz_after]\n"
		   "b = 2\n"
		   "\n"
		   "[5]\n"
		   "name = \"Lamp\"\n"
		   "[5.voe_game_glow]\n");

	// A kept section under a name that has since been registered.
	const voe_authoring_kept_section clash[] = {
		{ .id = 2, .key = "voe_scene_transform", .lines = "", .size = 0 },
	};

	text = "untouched";
	size = 12345;
	VOE_TEST_CHECK(!voe_authoring_scene_write(
		world, &(voe_authoring_kept){ .sections = clash, .count = 1 },
		arena, &text, &size));
	VOE_TEST_CHECK(strcmp(text, "untouched") == 0);

	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_three_entities();
	test_every_kind();
	test_floats();
	test_ascending_ids();
	test_entity_without_identity();
	test_empty_world();
	test_refusals();
	test_same_bytes_twice();
	test_kept_sections();
	return voe_test_result();
}
