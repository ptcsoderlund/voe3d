// The scene writer: that a world comes out as the exact bytes ADR-0149 names,
// the same bytes every time, and that each value a file could not carry back is
// refused rather than written.
//
// EVERY EXPECTATION IS A STRING LITERAL COMPARED BYTE FOR BYTE. Order, blank
// lines and the final newline are what make two saves of one scene diff clean,
// and a check that parsed the output would forgive exactly those.
//
// EVERY TEST-ONLY COMPONENT IS DECLARED THROUGH VOE_BASE_DESCRIBE_STRUCT, NOT
// WRITTEN BY HAND — since ADR-0154 every kind may be an array, so there is no
// shape here the macro cannot declare, and the macro is what the writer under
// test actually reads. `shapes` is the one built to prove nesting from rank 0
// to 7 (ADR-0154 points 1–4, 7, 8); `every`, `floats`, `link` and `shaped`
// were already here and are unchanged in what they hold, only in how their
// table is written.
//
// Refusals and warnings print a line to stderr; that is the report doing its job,
// not a failure.
#include <authoring/scene_write.h>

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <math/float2.h>
#include <math/float3.h>
#include <math/float4.h>
#include <math/float4x4.h>
#include <math/quat.h>
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

#define EVERY_FIELDS(F, F_READ_ONLY)          \
	F(int8_t, i8, INT8)                    \
	F(int16_t, i16, INT16)                 \
	F(int32_t, i32, INT32)                 \
	F(int64_t, i64, INT64)                 \
	F(uint8_t, u8, UINT8)                  \
	F(uint16_t, u16, UINT16)               \
	F(uint32_t, u32, UINT32)               \
	F(uint64_t, u64, UINT64)               \
	F(float, f32, FLOAT32)                 \
	F(double, f64, FLOAT64)                \
	F(bool, flag, BOOL)                    \
	F(voe_math_float2, f2, FLOAT2)         \
	F(voe_math_float3, f3, FLOAT3)         \
	F(voe_math_float4, f4, FLOAT4)         \
	F(voe_math_quat, q, QUAT)              \
	F(voe_math_float4x4, m, FLOAT4X4)      \
	F(voe_math_float3, points, FLOAT3, 2)  \
	F(char, label, CHAR, 16)               \
	F(voe_ecs_entity, target, ENTITY)      \
	F(voe_ecs_entity, pair, ENTITY, 2)

VOE_BASE_DESCRIBE_STRUCT(every, EVERY_FIELDS)

static const struct voe_ecs_key every_key = { "test_every" };
static const struct voe_ecs_key runtime_key = { "aaa_runtime" };

static void test_every_kind(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(
		world, &every_key, sizeof(every), ENTITIES, every_description());
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
		.m = voe_math_float4x4_identity(),
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

#define FLOATS_FIELDS(F, F_READ_ONLY) \
	F(float, f, FLOAT32)          \
	F(double, d, FLOAT64)

VOE_BASE_DESCRIBE_STRUCT(floats, FLOATS_FIELDS)

static const struct voe_ecs_key floats_key = { "test_floats" };

static voe_ecs_world *floats_world(voe_base_arena *arena, floats value)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(
		world, &floats_key, sizeof(floats), ENTITIES, floats_description());
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

#define LINK_FIELDS(F, F_READ_ONLY) \
	F(voe_ecs_entity, target, ENTITY)

VOE_BASE_DESCRIBE_STRUCT(link, LINK_FIELDS)

static const struct voe_ecs_key link_key = { "test_link" };

static void test_entity_without_identity(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(
		world, &link_key, sizeof(link), ENTITIES, link_description());
	voe_ecs_entity source = authored(world, 4, "Source");
	link value = { .target = entity_of(world) };
	size_t size;

	VOE_TEST_CHECK(voe_ecs_component_add(world, type, source, &value));

	const char *text = written(world, arena, &size);

	CHECK_TEXT(text, size,
		   "[4]\nname = \"Source\"\n[4.test_link]\ntarget = 0\n");

	voe_base_arena_destroy(arena);
}

// One test-only component proving every shape ADR-0154 names, from rank 0
// (every other component's fields) to 7 — a 2-by-3 grid of integers, a
// 1-dimensional array of vectors, a 2-by-2 grid of vectors nesting three
// brackets deep, an array of strings, a 7-dimensional array so thin every
// dimension but the last is 1, a 7-dimensional array of vectors nesting eight
// brackets deep, and an array of entity references.
#define SHAPES_FIELDS(F, F_READ_ONLY)                             \
	F(int32_t, pair, INT32, 2, 3)                              \
	F(voe_math_float3, points, FLOAT3, 2)                      \
	F(voe_math_float3, grid, FLOAT3, 2, 2)                     \
	F(char, tags, CHAR, 3, 8)                                  \
	F(uint8_t, deep, UINT8, 1, 1, 1, 1, 1, 1, 2)                \
	F(voe_math_float3, deepest, FLOAT3, 1, 1, 1, 1, 1, 1, 1)    \
	F(voe_ecs_entity, links, ENTITY, 2)

VOE_BASE_DESCRIBE_STRUCT(shapes, SHAPES_FIELDS)

static const struct voe_ecs_key shapes_key = { "test_shapes" };

static void test_shapes(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(
		world, &shapes_key, sizeof(shapes), ENTITIES, shapes_description());
	voe_ecs_entity ref = authored(world, 3, "Ref");
	voe_ecs_entity gone = entity_of(world);
	voe_ecs_entity one = authored(world, 1, "Shapes");
	size_t size;

	voe_ecs_entity_destroy(world, gone);

	shapes row = {
		.pair = { { 1, 2, 3 }, { 4, 5, 6 } },
		.points = { { 0, 0, 0 }, { 1, 2.5f, -3 } },
		.grid = { { { 0, 0, 0 }, { 1, 0, 0 } }, { { 2, 0, 0 }, { 3, 0, 0 } } },
		.tags = { "a", "x\"y\\z", "" },
		.deep = { { { { { { { 0, 7 } } } } } } },
		.deepest = { { { { { { { { 1, 2, 3 } } } } } } } },
		.links = { ref, gone },
	};

	VOE_TEST_CHECK(voe_ecs_component_add(world, type, one, &row));

	const char *text = written(world, arena, &size);

	CHECK_TEXT(text, size,
		   "[1]\n"
		   "name = \"Shapes\"\n"
		   "[1.test_shapes]\n"
		   "pair = [[1, 2, 3], [4, 5, 6]]\n"
		   "points = [[0, 0, 0], [1, 2.5, -3]]\n"
		   "grid = [[[0, 0, 0], [1, 0, 0]], [[2, 0, 0], [3, 0, 0]]]\n"
		   "tags = [\"a\", \"x\\\"y\\\\z\", \"\"]\n"
		   "deep = [[[[[[[0, 7]]]]]]]\n"
		   "deepest = [[[[[[[[1, 2, 3]]]]]]]]\n"
		   "links = [3, 0]\n"
		   "\n"
		   "[3]\n"
		   "name = \"Ref\"\n");

	voe_base_arena_destroy(arena);
}

// A tags element holding a byte below 0x20 refuses, naming tags[n] — checked
// only by the refusal (VOE_BASE_ERROR's text is not itself compared), which
// this exercises by giving element 1 a tab.
static void test_shapes_control_character(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(
		world, &shapes_key, sizeof(shapes), ENTITIES, shapes_description());
	voe_ecs_entity one = authored(world, 1, "Shapes");

	shapes row = {
		.pair = { { 1, 2, 3 }, { 4, 5, 6 } },
		.points = { { 0, 0, 0 }, { 1, 1, 1 } },
		.grid = { { { 0, 0, 0 }, { 0, 0, 0 } }, { { 0, 0, 0 }, { 0, 0, 0 } } },
		.tags = { "a", "b\tc", "" },
		.deep = { { { { { { { 0, 0 } } } } } } },
		.deepest = { { { { { { { { 0, 0, 0 } } } } } } } },
		.links = { 0 },
	};

	VOE_TEST_CHECK(voe_ecs_component_add(world, type, one, &row));
	check_refused(world, arena);

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

#define SHAPED_FIELDS(F, F_READ_ONLY) \
	F(shape, kind, ENUM)

VOE_BASE_DESCRIBE_STRUCT(shaped, SHAPED_FIELDS)

static const struct voe_ecs_key shaped_key = { "test_shaped" };

static void test_refusals(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	// An enum.
	{
		voe_ecs_world *world = world_of(arena);
		voe_ecs_type type = voe_ecs_component_register(
			world, &shaped_key, sizeof(shaped), ENTITIES,
			shaped_description());
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
	test_shapes();
	test_shapes_control_character();
	test_empty_world();
	test_refusals();
	test_same_bytes_twice();
	test_kept_sections();
	return voe_test_result();
}
