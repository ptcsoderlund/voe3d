// The scene reader: that a file becomes a world and goes back out as the same
// bytes, that a world goes out and comes back as the same rows, that a file wrong
// anywhere creates nothing, and that a section nobody registered survives the trip.
//
// A REFUSAL IS CHECKED BY THE WORLD HOLDING NO ENTITY AFTERWARDS, and every refused
// file puts its fault after at least one good section, so a reader that created as
// it went would be caught rather than forgiven.
//
// EVERY TEST-ONLY COMPONENT IS DECLARED THROUGH VOE_BASE_DESCRIBE_STRUCT, NOT
// WRITTEN BY HAND, since ADR-0154 lets every kind be an array and there is no
// shape left the macro cannot declare. `shapes` is the one built to prove
// reading every shape from rank 0 to 7 (ADR-0154 points 1–4, 7, 8), the same
// component scene_write.c's test declares, duplicated the way `link` already
// was between the two files — each test file is its own program.
//
// A WORLD THAT ALREADY HOLDS AN AUTHORED ENTITY ASSERTS, AND IS NOT TESTED HERE:
// voe::testing has no way to catch an assert, and a test that tripped one would
// end the program rather than report.
//
// Refusals and warnings print a line to stderr; that is the report doing its job,
// not a failure.
#include <authoring/scene_read.h>
#include <authoring/scene_write.h>

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <math/float3.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ENTITIES 16

#define LINK_FIELDS(F, F_READ_ONLY) \
	F(voe_ecs_entity, target, ENTITY)

VOE_BASE_DESCRIBE_STRUCT(link, LINK_FIELDS)

static const struct voe_ecs_key link_key = { "test_link" };

#define SAMPLE_FIELDS(F, F_READ_ONLY)             \
	F(bool, flag, BOOL)                        \
	F(int16_t, small, INT16)                   \
	F(voe_math_float3, points, FLOAT3, 2)       \
	F(char, label, CHAR, 16)                   \
	F(uint64_t, big, UINT64)                   \
	F(double, precise, FLOAT64)                \
	F(voe_ecs_entity, pair, ENTITY, 2)

VOE_BASE_DESCRIBE_STRUCT(sample, SAMPLE_FIELDS)

static const struct voe_ecs_key sample_key = { "test_sample" };

#define SHAPED_FIELDS(F, F_READ_ONLY) \
	F(int, kind, ENUM)

VOE_BASE_DESCRIBE_STRUCT(shaped, SHAPED_FIELDS)

static const struct voe_ecs_key shaped_key = { "test_shaped" };
static const struct voe_ecs_key runtime_key = { "test_runtime" };

// The same shapes component scene_write.c's test declares — see this file's
// header. Proves rank 0 to 7 on the read side: a 2-by-3 grid of integers, a
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

// A COLOUR field beside a plain float, with a default row giving both a non-zero
// value: proves COLOUR reads back byte for byte and that a missing field reads
// as its default (ADR-0190, ADR-0191).
#define TINTED_FIELDS(F, F_READ_ONLY)    \
	F(voe_math_float3, colour, COLOUR) \
	F(float, weight, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tinted, TINTED_FIELDS)

static const struct voe_ecs_key tinted_key = { "test_tinted" };

struct types {
	voe_ecs_type link;
	voe_ecs_type sample;
	voe_ecs_type shapes;
	voe_ecs_type tinted;
};

static voe_ecs_world *world_of(voe_base_arena *arena, uint32_t entities,
			       struct types *types)
{
	voe_ecs_world *world = voe_ecs_world_new(arena, (voe_ecs_limits){
		.entities = entities,
		.component_types = 12,
		.intent_types = 12,
	});

	voe_scene_transform_register(world, ENTITIES);
	voe_scene_identity_register(world, ENTITIES);
	voe_scene_camera_register(world, ENTITIES);
	types->link = voe_ecs_component_register(world, &link_key, sizeof(link),
						 ENTITIES, link_description());
	types->sample = voe_ecs_component_register(
		world, &sample_key, sizeof(sample), ENTITIES, sample_description());
	types->shapes = voe_ecs_component_register(
		world, &shapes_key, sizeof(shapes), ENTITIES, shapes_description());
	types->tinted = voe_ecs_component_register(
		world, &tinted_key, sizeof(tinted), ENTITIES, tinted_description());
	voe_ecs_component_default_set(world, types->tinted, &(tinted){
		.colour = { 0.7f, 0.7f, 0.7f },
		.weight = 2.0f,
	});
	(void)voe_ecs_component_register(world, &shaped_key, sizeof(shaped),
					 ENTITIES, shaped_description());
	(void)voe_ecs_component_register(world, &runtime_key, sizeof(int),
					 ENTITIES, &voe_ecs_runtime_only);
	return world;
}

// The entity with this authored id, or a zeroed one.
static voe_ecs_entity entity_with_id(const voe_ecs_world *world, uint64_t id)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(world);
	const voe_ecs_entity *entities = voe_scene_identity_entities(world);

	for (uint32_t i = 0; i < voe_scene_identity_count(world); i++)
		if (rows[i].id == id)
			return entities[i];
	return (voe_ecs_entity){ 0 };
}

// The authored id an entity reference points at, or 0.
static uint64_t id_of(const voe_ecs_world *world, voe_ecs_entity entity)
{
	const voe_scene_identity *identity = voe_scene_identity_get(world, entity);

	return identity != NULL ? identity->id : 0;
}

static void check_text(const char *actual, size_t size, const char *expected,
		       const char *file, int line)
{
	if (size == strlen(expected) && memcmp(actual, expected, size) == 0)
		return;

	voe_test_report(file, line, "the written text == the expected text");
	fprintf(stderr, "      actual (%zu bytes):\n%.*s\n      expected (%zu bytes):\n%s\n",
		size, (int)size, actual, strlen(expected), expected);
}

#define CHECK_TEXT(actual, size, expected) \
	check_text((actual), (size), (expected), __FILE__, __LINE__)

static bool read_text(const char *text, voe_ecs_world *world,
		      voe_base_arena *arena, voe_authoring_kept *kept)
{
	return voe_authoring_scene_read(text, strlen(text), world, arena, kept);
}

static void check_round_trip(const char *text, const char *file, int line)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	struct types types;
	voe_ecs_world *world = world_of(arena, ENTITIES, &types);
	voe_authoring_kept kept = { 0 };
	voe_authoring_text out = { .text = "", .size = 0 };

	if (!read_text(text, world, arena, &kept))
		voe_test_report(file, line, "the text was read");
	else if (!voe_authoring_scene_write(world, &kept, arena, &out))
		voe_test_report(file, line, "the world was written");
	else
		check_text(out.text, out.size, text, file, line);

	voe_base_arena_destroy(arena);
}

#define CHECK_ROUND_TRIP(text) check_round_trip((text), __FILE__, __LINE__)

static const char *const canonical =
	"[1]\n"
	"name = \"Camera\"\n"
	"[1.voe_scene_camera]\n"
	"fov_y = 1.25\n"
	"near_plane = 0.1\n"
	"far_plane = 100\n"
	"[1.voe_scene_transform]\n"
	"position = [0, 2, 5]\n"
	"rotation = [0, 0, 0, 1]\n"
	"scale = [1, 1, 1]\n"
	"\n"
	"[2]\n"
	"name = \"Cube \\\"big\\\"\"\n"
	"[2.test_link]\n"
	"target = 3\n"
	"[2.voe_game_health]\n"
	"points = 20\n"
	"armour = \"light\"\n"
	"[2.voe_scene_transform]\n"
	"position = [1, 0.5, -3]\n"
	"rotation = [0, 0.5, 0, 0.75]\n"
	"scale = [2, 2, 2]\n"
	"\n"
	"[3]\n"
	"name = \"Floor\"\n"
	"[3.test_link]\n"
	"target = 0\n";

static void test_round_trip_text_first(void)
{
	CHECK_ROUND_TRIP(canonical);

	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	struct types types;
	voe_ecs_world *world = world_of(arena, ENTITIES, &types);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(read_text(canonical, world, arena, &kept));
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), 3);
	VOE_TEST_CHECK_INT(kept.count, 1);
	if (kept.count == 1) {
		VOE_TEST_CHECK_INT(kept.sections[0].id, 2);
		VOE_TEST_CHECK(strcmp(kept.sections[0].key, "voe_game_health") == 0);
		CHECK_TEXT(kept.sections[0].lines, kept.sections[0].size,
			   "points = 20\narmour = \"light\"\n");
	}

	voe_ecs_entity cube = entity_with_id(world, 2);
	voe_ecs_entity floor = entity_with_id(world, 3);
	const link *reference = voe_ecs_component_get(world, types.link, cube);
	const voe_scene_camera *camera =
		voe_scene_camera_get(world, entity_with_id(world, 1));

	VOE_TEST_CHECK(reference != NULL && reference->target.index == floor.index &&
		       reference->target.generation == floor.generation);
	VOE_TEST_CHECK(camera != NULL && camera->fov_y == 1.25f &&
		       camera->far_plane == 100.0f);
	VOE_TEST_CHECK(strcmp(voe_scene_identity_get(world, cube)->name,
			      "Cube \"big\"") == 0);

	voe_base_arena_destroy(arena);
}

// Every row of every described type in `a`, field by field, against the entity
// with the same authored id in `b`; a reference is compared by the id it names.
static void check_same_rows(const voe_ecs_world *a, const voe_ecs_world *b)
{
	const voe_scene_identity *identities = voe_scene_identity_rows(a);
	const voe_ecs_entity *entities = voe_scene_identity_entities(a);

	VOE_TEST_CHECK_INT(voe_scene_identity_count(b), voe_scene_identity_count(a));
	for (uint32_t t = 0; t < voe_ecs_component_type_count(a); t++) {
		voe_ecs_type type = voe_ecs_component_type_at(a, t);
		const voe_base_struct_description *description =
			voe_ecs_component_description(a, type);

		if (description == NULL)
			continue;
		for (uint32_t i = 0; i < voe_scene_identity_count(a); i++) {
			voe_ecs_entity there = entity_with_id(b, identities[i].id);
			const uint8_t *row_a =
				voe_ecs_component_get(a, type, entities[i]);
			const uint8_t *row_b = voe_ecs_component_get(b, type, there);

			VOE_TEST_CHECK((row_a == NULL) == (row_b == NULL));
			if (row_a == NULL || row_b == NULL)
				continue;
			for (uint32_t f = 0; f < description->field_count; f++) {
				const voe_base_field_description *field =
					&description->fields[f];

				if (field->kind != VOE_BASE_FIELD_ENTITY) {
					VOE_TEST_CHECK(memcmp(row_a + field->offset,
							      row_b + field->offset,
							      field->size) == 0);
					continue;
				}
				for (uint32_t e = 0; e < field->count; e++) {
					voe_ecs_entity ea;
					voe_ecs_entity eb;

					memcpy(&ea, row_a + field->offset + e * sizeof(ea),
					       sizeof(ea));
					memcpy(&eb, row_b + field->offset + e * sizeof(eb),
					       sizeof(eb));
					VOE_TEST_CHECK_INT(id_of(b, eb), id_of(a, ea));
				}
			}
		}
	}
}

static voe_ecs_entity authored(voe_ecs_world *world, uint64_t id,
			       const char *name)
{
	voe_ecs_entity entity = { 0 };
	voe_scene_identity identity = { .id = id };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	memcpy(identity.name, name, strlen(name));
	VOE_TEST_CHECK(voe_scene_identity_add(world, entity, identity));
	return entity;
}

static void test_round_trip_world_first(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	struct types types;
	voe_ecs_world *first = world_of(arena, ENTITIES, &types);
	voe_ecs_entity sun = authored(first, 9, "Sun");
	voe_ecs_entity cube = authored(first, 2, "Cube");
	voe_ecs_entity eye = authored(first, 5, "Eye");
	voe_ecs_entity probe = { 0 };
	voe_authoring_text out = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(first, &probe));
	VOE_TEST_CHECK(voe_scene_transform_add(first, cube, (voe_scene_transform){
		.position = { 0.1, -2.5, 1.0 / 3.0 },
		.rotation = { 0.0f, 0.38268343f, 0.0f, 0.9238795f },
		.scale = { 1.0f, 1.0f, 1.0f },
	}));
	VOE_TEST_CHECK(voe_scene_transform_add(first, sun, (voe_scene_transform){
		.position = { 0.0f, 100.0f, 0.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	}));
	VOE_TEST_CHECK(voe_scene_transform_add(first, eye, (voe_scene_transform){
		.position = { 0.0, 1.7, 4.0 },
		.rotation = { -0.09933467f, 0.04983342f, 0.0049667f, 0.99374009f },
		.scale = { 1.0f, 1.0f, 1.0f },
	}));
	VOE_TEST_CHECK(voe_scene_camera_add(first, eye, (voe_scene_camera){
		.fov_y = 1.0471976f,
		.near_plane = 0.05f,
		.far_plane = 1000.0f,
	}));
	VOE_TEST_CHECK(voe_ecs_component_add(first, types.link, cube,
					     &(link){ .target = sun }));
	VOE_TEST_CHECK(voe_ecs_component_add(first, types.link, sun,
					     &(link){ .target = probe }));
	VOE_TEST_CHECK(voe_ecs_component_add(first, types.sample, eye, &(sample){
		.flag = true,
		.small = -1234,
		.points = { { 0.0f, 1.0f, 2.0f }, { -1e-5f, 3.4028235e38f, 0.5f } },
		.label = "a \\ \"b\"",
		.big = UINT64_MAX,
		.precise = 1e-300,
		.pair = { cube, eye },
	}));

	VOE_TEST_CHECK(voe_authoring_scene_write(first, NULL, arena, &out));

	struct types second_types;
	voe_ecs_world *second = world_of(arena, ENTITIES, &second_types);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(out.text != NULL &&
		       voe_authoring_scene_read(out.text, out.size, second, arena,
						&kept));
	VOE_TEST_CHECK_INT(kept.count, 0);
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(second), 3);
	check_same_rows(first, second);

	voe_base_arena_destroy(arena);
}

// A position 100 km out is written, read back into a fresh world and is the
// same bits, as undo and Play need (ADR-0250).
static void test_far_position_round_trip(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	struct types types;
	voe_ecs_world *first = world_of(arena, ENTITIES, &types);
	const voe_math_double3 far = { 100000.123456789, -0.05, 1e7 + 0.001 };
	voe_authoring_text out = { 0 };

	VOE_TEST_CHECK(voe_scene_transform_add(first, authored(first, 4, "Far"),
					       (voe_scene_transform){
		.position = far,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	}));
	VOE_TEST_CHECK(voe_authoring_scene_write(first, NULL, arena, &out));

	struct types second_types;
	voe_ecs_world *second = world_of(arena, ENTITIES, &second_types);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(out.text != NULL &&
		       voe_authoring_scene_read(out.text, out.size, second, arena,
						&kept));
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(second), 1);

	const voe_scene_transform *back =
		voe_scene_transform_get(second, entity_with_id(second, 4));

	VOE_TEST_CHECK(back != NULL &&
		       memcmp(&back->position, &far, sizeof(far)) == 0);
	voe_base_arena_destroy(arena);
}

// A COLOUR field goes out as three numbers, as FLOAT3 does, and comes back byte
// for byte.
static void test_colour_round_trip(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	struct types types;
	voe_ecs_world *first = world_of(arena, ENTITIES, &types);
	voe_ecs_entity cube = authored(first, 1, "Cube");
	tinted row = { .colour = { 0.25f, 0.1f, 1.0f }, .weight = 0.5f };
	voe_authoring_text out = { 0 };

	VOE_TEST_CHECK(voe_ecs_component_add(first, types.tinted, cube, &row));
	VOE_TEST_CHECK(voe_authoring_scene_write(first, NULL, arena, &out));
	CHECK_TEXT(out.text, out.size,
		   "[1]\n"
		   "name = \"Cube\"\n"
		   "[1.test_tinted]\n"
		   "colour = [0.25, 0.1, 1]\n"
		   "weight = 0.5\n");

	struct types second_types;
	voe_ecs_world *second = world_of(arena, ENTITIES, &second_types);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(out.text != NULL &&
		       voe_authoring_scene_read(out.text, out.size, second, arena,
						&kept));

	const tinted *back = voe_ecs_component_get(
		second, second_types.tinted, entity_with_id(second, 1));

	VOE_TEST_CHECK(back != NULL && memcmp(back, &row, sizeof(row)) == 0);

	voe_base_arena_destroy(arena);
}

static void check_refused(const char *text, const char *file, int line)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	struct types types;
	voe_ecs_world *world = world_of(arena, ENTITIES, &types);
	const voe_authoring_kept sentinel = { .count = 12345 };
	voe_authoring_kept kept = sentinel;

	if (read_text(text, world, arena, &kept))
		voe_test_report(file, line, "the text was refused");
	if (voe_ecs_entity_count(world) != 0)
		voe_test_report(file, line, "a refused text created no entity");
	if (kept.count != sentinel.count || kept.sections != NULL)
		voe_test_report(file, line, "a refused text left kept untouched");

	voe_base_arena_destroy(arena);
}

#define CHECK_REFUSED(text) check_refused((text), __FILE__, __LINE__)

// A good entity and a good transform, before every fault below.
#define GOOD "[1]\nname = \"ok\"\n[1.voe_scene_transform]\n" \
	     "position = [0, 0, 0]\nrotation = [0, 0, 0, 1]\nscale = [1, 1, 1]\n\n"

static void test_refusals(void)
{
	// A bad float.
	CHECK_REFUSED(GOOD "[2]\nname = \"b\"\n[2.voe_scene_transform]\n"
			   "position = [0, 1.2.3, 0]\nrotation = [0, 0, 0, 1]\n"
			   "scale = [1, 1, 1]\n");
	CHECK_REFUSED(GOOD "[2]\nname = \"b\"\n[2.voe_scene_camera]\nfov_y = nan\n"
			   "[2.voe_scene_transform]\n"
			   "position = [0, 0, 0]\nrotation = [0, 0, 0, 1]\n"
			   "scale = [1, 1, 1]\n");
	CHECK_REFUSED(GOOD "[2]\nname = \"b\"\n[2.voe_scene_camera]\nfov_y = 1e39\n"
			   "[2.voe_scene_transform]\n"
			   "position = [0, 0, 0]\nrotation = [0, 0, 0, 1]\n"
			   "scale = [1, 1, 1]\n");
	// bool = 1.
	CHECK_REFUSED(GOOD "[2]\nname = \"b\"\n[2.test_sample]\nflag = 1\n");
	// A vector with two numbers.
	CHECK_REFUSED(GOOD "[2]\nname = \"b\"\n[2.voe_scene_transform]\n"
			   "position = [0, 0]\n");
	// An escape the format does not have, which the sectioned reader refuses.
	CHECK_REFUSED(GOOD "[2]\nname = \"C:\\x\"\n");
	// A string one byte too long: 64 bytes, and the name holds 63.
	CHECK_REFUSED(GOOD "[2]\nname = \"0123456789012345678901234567890123456789"
			   "012345678901234567890123\"\n");
	// Section names that are not a scene's.
	CHECK_REFUSED(GOOD "[0]\nname = \"zero\"\n");
	CHECK_REFUSED(GOOD "[x]\nname = \"x\"\n");
	CHECK_REFUSED(GOOD "[02]\nname = \"leading zero\"\n");
	CHECK_REFUSED(GOOD "[4.voe_scene_transform]\nposition = [0, 0, 0]\n");
	CHECK_REFUSED(GOOD "[1.voe_scene_identity]\nname = \"twice\"\n");
	// A runtime-only type's section.
	CHECK_REFUSED(GOOD "[1.test_runtime]\nvalue = 1\n");
	// An enum.
	CHECK_REFUSED(GOOD "[1.test_shaped]\nkind = 1\n");
	// Integers out of range, and a negative unsigned.
	CHECK_REFUSED(GOOD "[1.test_sample]\nsmall = 32768\n");
	CHECK_REFUSED(GOOD "[1.test_sample]\nbig = -1\n");
	// A fixed array of a vector kind with the wrong count, outside and in.
	CHECK_REFUSED(GOOD "[1.test_sample]\npoints = [[0, 0, 0]]\n");
	CHECK_REFUSED(GOOD "[1.test_sample]\npoints = [[0, 0], [1, 1, 1]]\n");
	CHECK_REFUSED(GOOD "[1.test_sample]\npoints = [[0, 0, 0], [1, 1, 1]] x\n");
}

// Every way a field's own shape (ADR-0154) may be wrong: a row too short, a
// row flattened, a row mixed with another kind, a bad escape inside an array
// string, a string one byte too long for its slot inside an array, nesting
// past the 8-level limit, and an empty array.
static void test_shapes_refusals(void)
{
	// pair: a row too short, flattened, and mixed with a string.
	CHECK_REFUSED(GOOD "[1.test_shapes]\npair = [[1, 2, 3], [4, 5]]\n");
	CHECK_REFUSED(GOOD "[1.test_shapes]\npair = [1, 2, 3, 4, 5, 6]\n");
	CHECK_REFUSED(GOOD "[1.test_shapes]\npair = [[1, 2, 3], [4, 5, \"6\"]]\n");
	// tags: a backslash escape the format does not have, inside an array —
	// the sectioned reader does not check this text at all, since the
	// value as a whole does not start with '"' (it starts with '[').
	CHECK_REFUSED(GOOD "[1.test_shapes]\ntags = [\"a\", \"C:\\x\", \"\"]\n");
	// tags: 8 bytes in a slot that holds 7 beside the terminating zero.
	CHECK_REFUSED(GOOD "[1.test_shapes]\ntags = [\"12345678\", \"b\", \"\"]\n");
	// deepest: nine levels of brackets, one past BRACKET_DEPTH_MAX.
	CHECK_REFUSED(GOOD "[1.test_shapes]\ndeepest = [[[[[[[[[1, 2, 3]]]]]]]]]\n");
	// points: an empty array where the field wants two vectors.
	CHECK_REFUSED(GOOD "[1.test_shapes]\npoints = []\n");
}

// Blanks inside a field's own brackets load, and are written back canonical.
static void test_shapes_tolerated_spacing(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	struct types types;
	voe_ecs_world *world = world_of(arena, ENTITIES, &types);
	voe_authoring_kept kept = { 0 };
	voe_authoring_text out = { 0 };

	VOE_TEST_CHECK(read_text("[1]\n"
				 "name = \"a\"\n"
				 "[1.test_shapes]\n"
				 "pair = [ [1,2,3] ,[4, 5,6] ]\n"
				 "points = [[0, 0, 0], [1, 2.5, -3]]\n"
				 "grid = [[[0, 0, 0], [1, 0, 0]], [[2, 0, 0], [3, 0, 0]]]\n"
				 "tags = [\"a\", \"x\\\"y\\\\z\", \"\"]\n"
				 "deep = [[[[[[[0, 7]]]]]]]\n"
				 "deepest = [[[[[[[[1, 2, 3]]]]]]]]\n"
				 "links = [0, 0]\n",
				 world, arena, &kept));

	VOE_TEST_CHECK(voe_authoring_scene_write(world, &kept, arena, &out));
	CHECK_TEXT(out.text, out.size,
		   "[1]\n"
		   "name = \"a\"\n"
		   "[1.test_shapes]\n"
		   "pair = [[1, 2, 3], [4, 5, 6]]\n"
		   "points = [[0, 0, 0], [1, 2.5, -3]]\n"
		   "grid = [[[0, 0, 0], [1, 0, 0]], [[2, 0, 0], [3, 0, 0]]]\n"
		   "tags = [\"a\", \"x\\\"y\\\\z\", \"\"]\n"
		   "deep = [[[[[[[0, 7]]]]]]]\n"
		   "deepest = [[[[[[[[1, 2, 3]]]]]]]]\n"
		   "links = [0, 0]\n");

	voe_base_arena_destroy(arena);
}

static void test_warnings(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	struct types types;
	voe_ecs_world *world = world_of(arena, ENTITIES, &types);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(read_text("[1]\n"
				 "name = \"a\"\n"
				 "colour = \"red\"\n"
				 "[1.voe_scene_transform]\n"
				 "position = [1, 2, 3]\n"
				 "rotation = [0, 0, 0, 1]\n"
				 "[1.test_link]\n"
				 "target = 77\n"
				 "[1.test_sample]\n"
				 "flag = true\n"
				 "[1.test_tinted]\n"
				 "weight = 3\n",
				 world, arena, &kept));

	voe_ecs_entity entity = entity_with_id(world, 1);
	const voe_scene_transform *transform = voe_scene_transform_get(world, entity);
	const link *reference = voe_ecs_component_get(world, types.link, entity);
	const sample *plain = voe_ecs_component_get(world, types.sample, entity);
	const tinted *tint = voe_ecs_component_get(world, types.tinted, entity);

	// A missing field reads as the type's default row: transform's scale is 1,
	// tinted's colour its grey; sample has no default, so its missing fields
	// read zero.
	VOE_TEST_CHECK(transform != NULL && transform->position.y == 2.0f &&
		       transform->scale.x == 1.0f && transform->scale.z == 1.0f);
	VOE_TEST_CHECK(tint != NULL && tint->weight == 3.0f &&
		       tint->colour.x == 0.7f && tint->colour.y == 0.7f &&
		       tint->colour.z == 0.7f);
	VOE_TEST_CHECK(plain != NULL && plain->flag && plain->small == 0 &&
		       plain->big == 0 && plain->precise == 0.0 &&
		       plain->label[0] == '\0');
	VOE_TEST_CHECK(reference != NULL && reference->target.index == 0 &&
		       reference->target.generation == 0);
	VOE_TEST_CHECK(strcmp(voe_scene_identity_get(world, entity)->name, "a") == 0);

	voe_base_arena_destroy(arena);
}

static void test_file_order(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	struct types types;
	voe_ecs_world *world = world_of(arena, ENTITIES, &types);
	voe_authoring_kept kept = { 0 };
	voe_authoring_text out = { .text = "", .size = 0 };

	VOE_TEST_CHECK(read_text("[3]\nname = \"c\"\n[3.test_link]\ntarget = 1\n\n"
				 "[1]\nname = \"a\"\n\n"
				 "[2]\nname = \"b\"\n",
				 world, arena, &kept));
	VOE_TEST_CHECK(voe_authoring_scene_write(world, &kept, arena, &out));
	CHECK_TEXT(out.text, out.size,
		   "[1]\nname = \"a\"\n\n[2]\nname = \"b\"\n\n"
		   "[3]\nname = \"c\"\n[3.test_link]\ntarget = 1\n");

	voe_base_arena_destroy(arena);
}

static void test_fixed_arrays(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	struct types types;
	voe_ecs_world *world = world_of(arena, ENTITIES, &types);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(read_text("[1]\nname = \"a\"\n"
				 "[1.test_sample]\n"
				 "flag = true\n"
				 "small = -32768\n"
				 "points = [[0, 0, 0],[1,1, 1]]\n"
				 "label = \"hi\"\n"
				 "big = 18446744073709551615\n"
				 "precise = 0.1\n"
				 "pair = [1, 0]\n",
				 world, arena, &kept));

	voe_ecs_entity entity = entity_with_id(world, 1);
	const sample *row = voe_ecs_component_get(world, types.sample, entity);

	VOE_TEST_CHECK(row != NULL);
	if (row != NULL) {
		VOE_TEST_CHECK(row->flag);
		VOE_TEST_CHECK_INT(row->small, -32768);
		VOE_TEST_CHECK(row->points[0].z == 0.0f && row->points[1].x == 1.0f &&
			       row->points[1].z == 1.0f);
		VOE_TEST_CHECK(strcmp(row->label, "hi") == 0);
		VOE_TEST_CHECK(row->big == UINT64_MAX);
		VOE_TEST_CHECK(row->precise == 0.1);
		VOE_TEST_CHECK(row->pair[0].index == entity.index &&
			       row->pair[0].generation == entity.generation);
		VOE_TEST_CHECK(row->pair[1].generation == 0);
	}

	voe_base_arena_destroy(arena);
}

// The shapes component on two entities, links naming each other and the
// second slot dead — exercises every shape ADR-0154 names, both directions.
static const char *const shapes_canonical =
	"[1]\n"
	"name = \"A\"\n"
	"[1.test_shapes]\n"
	"pair = [[1, 2, 3], [4, 5, 6]]\n"
	"points = [[0, 0, 0], [1, 2.5, -3]]\n"
	"grid = [[[0, 0, 0], [1, 0, 0]], [[2, 0, 0], [3, 0, 0]]]\n"
	"tags = [\"a\", \"x\\\"y\\\\z\", \"\"]\n"
	"deep = [[[[[[[0, 7]]]]]]]\n"
	"deepest = [[[[[[[[1, 2, 3]]]]]]]]\n"
	"links = [2, 0]\n"
	"\n"
	"[2]\n"
	"name = \"B\"\n"
	"[2.test_shapes]\n"
	"pair = [[1, 2, 3], [4, 5, 6]]\n"
	"points = [[0, 0, 0], [1, 2.5, -3]]\n"
	"grid = [[[0, 0, 0], [1, 0, 0]], [[2, 0, 0], [3, 0, 0]]]\n"
	"tags = [\"a\", \"x\\\"y\\\\z\", \"\"]\n"
	"deep = [[[[[[[0, 7]]]]]]]\n"
	"deepest = [[[[[[[[1, 2, 3]]]]]]]]\n"
	"links = [1, 0]\n";

static void test_shapes_round_trip_text_first(void)
{
	CHECK_ROUND_TRIP(shapes_canonical);
}

static void test_shapes_round_trip_world_first(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	struct types types;
	voe_ecs_world *first = world_of(arena, ENTITIES, &types);
	voe_ecs_entity a = authored(first, 1, "A");
	voe_ecs_entity b = authored(first, 2, "B");
	voe_authoring_text out = { 0 };

	shapes row = {
		.pair = { { 1, 2, 3 }, { 4, 5, 6 } },
		.points = { { 0, 0, 0 }, { 1, 2.5f, -3 } },
		.grid = { { { 0, 0, 0 }, { 1, 0, 0 } }, { { 2, 0, 0 }, { 3, 0, 0 } } },
		.tags = { "a", "x\"y\\z", "" },
		.deep = { { { { { { { 0, 7 } } } } } } },
		.deepest = { { { { { { { { 1, 2, 3 } } } } } } } },
	};
	shapes row_a = row;
	shapes row_b = row;

	row_a.links[0] = b;
	row_b.links[0] = a;

	VOE_TEST_CHECK(voe_ecs_component_add(first, types.shapes, a, &row_a));
	VOE_TEST_CHECK(voe_ecs_component_add(first, types.shapes, b, &row_b));

	VOE_TEST_CHECK(voe_authoring_scene_write(first, NULL, arena, &out));

	struct types second_types;
	voe_ecs_world *second = world_of(arena, ENTITIES, &second_types);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(out.text != NULL &&
		       voe_authoring_scene_read(out.text, out.size, second, arena,
						&kept));
	VOE_TEST_CHECK_INT(kept.count, 0);
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(second), 2);
	check_same_rows(first, second);

	voe_base_arena_destroy(arena);
}

static void test_world_runs_out(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	struct types types;
	voe_ecs_world *world = world_of(arena, 1, &types);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(!read_text("[1]\nname = \"a\"\n\n[2]\nname = \"b\"\n", world,
				  arena, &kept));
	VOE_TEST_CHECK_INT(kept.count, 0);

	voe_base_arena_destroy(arena);
}

static void test_empty_text(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	struct types types;
	voe_ecs_world *world = world_of(arena, ENTITIES, &types);
	voe_authoring_kept kept = { .count = 9 };

	VOE_TEST_CHECK(voe_authoring_scene_read("", 0, world, arena, &kept));
	VOE_TEST_CHECK_INT(kept.count, 0);
	VOE_TEST_CHECK(kept.sections == NULL);
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), 0);

	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_round_trip_text_first();
	test_round_trip_world_first();
	test_far_position_round_trip();
	test_colour_round_trip();
	test_refusals();
	test_shapes_refusals();
	test_shapes_tolerated_spacing();
	test_warnings();
	test_file_order();
	test_fixed_arrays();
	test_shapes_round_trip_text_first();
	test_shapes_round_trip_world_first();
	test_world_runs_out();
	test_empty_text();
	return voe_test_result();
}
