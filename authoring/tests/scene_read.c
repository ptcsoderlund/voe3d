// The scene reader: that a file becomes a world and goes back out as the same
// bytes, that a world goes out and comes back as the same rows, that a file wrong
// anywhere creates nothing, and that a section nobody registered survives the trip.
//
// A REFUSAL IS CHECKED BY THE WORLD HOLDING NO ENTITY AFTERWARDS, and every refused
// file puts its fault after at least one good section, so a reader that created as
// it went would be caught rather than forgiven.
//
// THE SAMPLE COMPONENT'S DESCRIPTION IS WRITTEN BY HAND, as the writer's every-kind
// test does and for the same reason: describe.h lets only ENUM, CHAR and ENTITY
// repeat, and a fixed array of a vector kind still has to be proven.
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

typedef struct {
	bool flag;
	int16_t small;
	float points[2][3];
	char label[16];
	uint64_t big;
	double precise;
	voe_ecs_entity pair[2];
} sample;

#define ROW(field, KIND, elements)                                            \
	{                                                                     \
		.name = #field,                                               \
		.kind = VOE_BASE_FIELD_##KIND,                                \
		.offset = offsetof(sample, field),                            \
		.size = sizeof(((sample *)0)->field),                         \
		.count = (elements),                                          \
	}

static const voe_base_field_description sample_rows[] = {
	ROW(flag, BOOL, 1),    ROW(small, INT16, 1),  ROW(points, FLOAT3, 2),
	ROW(label, CHAR, 16),  ROW(big, UINT64, 1),   ROW(precise, FLOAT64, 1),
	ROW(pair, ENTITY, 2),
};

static const voe_base_struct_description sample_description = {
	.name = "sample",
	.fields = sample_rows,
	.field_count = sizeof(sample_rows) / sizeof(sample_rows[0]),
};

static const struct voe_ecs_key sample_key = { "test_sample" };

typedef struct {
	int kind;
} shaped;

static const voe_base_field_description shaped_rows[] = {
	{ .name = "kind", .kind = VOE_BASE_FIELD_ENUM,
	  .offset = offsetof(shaped, kind), .size = sizeof(int), .count = 1 },
};

static const voe_base_struct_description shaped_description = {
	.name = "shaped",
	.fields = shaped_rows,
	.field_count = 1,
};

static const struct voe_ecs_key shaped_key = { "test_shaped" };
static const struct voe_ecs_key runtime_key = { "test_runtime" };

struct types {
	voe_ecs_type link;
	voe_ecs_type sample;
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
						 ENTITIES, &link_description);
	types->sample = voe_ecs_component_register(
		world, &sample_key, sizeof(sample), ENTITIES, &sample_description);
	(void)voe_ecs_component_register(world, &shaped_key, sizeof(shaped),
					 ENTITIES, &shaped_description);
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
	const char *out = "";
	size_t size = 0;

	if (!read_text(text, world, arena, &kept))
		voe_test_report(file, line, "the text was read");
	else if (!voe_authoring_scene_write(world, &kept, arena, &out, &size))
		voe_test_report(file, line, "the world was written");
	else
		check_text(out, size, text, file, line);

	voe_base_arena_destroy(arena);
}

#define CHECK_ROUND_TRIP(text) check_round_trip((text), __FILE__, __LINE__)

static const char *const canonical =
	"[1]\n"
	"name = \"Camera\"\n"
	"[1.voe_scene_camera]\n"
	"eye = [0, 2, 5]\n"
	"yaw = 0\n"
	"pitch = -0.25\n"
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
	VOE_TEST_CHECK(camera != NULL && camera->pitch == -0.25f &&
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
	const char *text = NULL;
	size_t size = 0;

	VOE_TEST_CHECK(voe_ecs_entity_create(first, &probe));
	VOE_TEST_CHECK(voe_scene_transform_add(first, cube, (voe_scene_transform){
		.position = { 0.1f, -2.5f, 1.0f / 3.0f },
		.rotation = { 0.0f, 0.38268343f, 0.0f, 0.9238795f },
		.scale = { 1.0f, 1.0f, 1.0f },
	}));
	VOE_TEST_CHECK(voe_scene_transform_add(first, sun, (voe_scene_transform){
		.position = { 0.0f, 100.0f, 0.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	}));
	VOE_TEST_CHECK(voe_scene_camera_add(first, eye, (voe_scene_camera){
		.eye = { 0.0f, 1.7f, 4.0f },
		.yaw = 0.1f,
		.pitch = -0.2f,
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

	VOE_TEST_CHECK(voe_authoring_scene_write(first, NULL, arena, &text, &size));

	struct types second_types;
	voe_ecs_world *second = world_of(arena, ENTITIES, &second_types);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(text != NULL &&
		       voe_authoring_scene_read(text, size, second, arena, &kept));
	VOE_TEST_CHECK_INT(kept.count, 0);
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(second), 3);
	check_same_rows(first, second);

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
	CHECK_REFUSED(GOOD "[2]\nname = \"b\"\n[2.voe_scene_camera]\nyaw = nan\n");
	CHECK_REFUSED(GOOD "[2]\nname = \"b\"\n[2.voe_scene_camera]\nyaw = 1e39\n");
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
				 "target = 77\n",
				 world, arena, &kept));

	voe_ecs_entity entity = entity_with_id(world, 1);
	const voe_scene_transform *transform = voe_scene_transform_get(world, entity);
	const link *reference = voe_ecs_component_get(world, types.link, entity);

	VOE_TEST_CHECK(transform != NULL && transform->position.y == 2.0f &&
		       transform->scale.x == 0.0f && transform->scale.z == 0.0f);
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
	const char *text = "";
	size_t size = 0;

	VOE_TEST_CHECK(read_text("[3]\nname = \"c\"\n[3.test_link]\ntarget = 1\n\n"
				 "[1]\nname = \"a\"\n\n"
				 "[2]\nname = \"b\"\n",
				 world, arena, &kept));
	VOE_TEST_CHECK(voe_authoring_scene_write(world, &kept, arena, &text, &size));
	CHECK_TEXT(text, size,
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
		VOE_TEST_CHECK(row->points[0][2] == 0.0f && row->points[1][0] == 1.0f &&
			       row->points[1][2] == 1.0f);
		VOE_TEST_CHECK(strcmp(row->label, "hi") == 0);
		VOE_TEST_CHECK(row->big == UINT64_MAX);
		VOE_TEST_CHECK(row->precise == 0.1);
		VOE_TEST_CHECK(row->pair[0].index == entity.index &&
			       row->pair[0].generation == entity.generation);
		VOE_TEST_CHECK(row->pair[1].generation == 0);
	}

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
	test_refusals();
	test_warnings();
	test_file_order();
	test_fixed_arrays();
	test_world_runs_out();
	test_empty_text();
	return voe_test_result();
}
