// The cook: that a world comes out as the exact C source authoring/scene_cook.h
// promises, that the source compiles against the engine's headers, and that
// each value it could not cook exactly is refused rather than cooked.
//
// THE COMPILE CHECK RUNS clang THROUGH system(), writing two files into the
// working directory: a header standing in for the game's (it includes scene's
// four component headers and declares the function) and the cooked source. The
// engine's include folders are found from __FILE__, three levels up.
//
// Refusals print a line to stderr; that is the report doing its job, not a
// failure.
#include <authoring/scene_cook.h>

#include <base/arena.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
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

	voe_test_report(file, line, "the cooked text == the expected text");
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
	voe_scene_light_register(world, ENTITIES);
	voe_scene_camera_register(world, ENTITIES);
	return world;
}

static voe_ecs_entity authored(voe_ecs_world *world, uint64_t id,
			       const char *name)
{
	voe_ecs_entity entity = { 0 };
	voe_scene_identity identity = { .id = id };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(strlen(name) < VOE_SCENE_IDENTITY_NAME);
	memcpy(identity.name, name, strlen(name));
	VOE_TEST_CHECK(voe_scene_identity_add(world, entity, identity));
	return entity;
}

// Two entities out of id order: a light, then a transform with 0.1f and -0.0f
// and a camera.
static voe_ecs_world *small_world(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity sun = authored(world, 7, "Sun \"key\"");
	voe_ecs_entity cube = authored(world, 3, "Cube");

	VOE_TEST_CHECK(voe_scene_light_add(world, sun, (voe_scene_light){
		.colour = { 1.0f, 0.5f, 0.25f },
		.intensity = 3.0f,
		.fill_colour = { 1.0f, 1.0f, 1.0f },
		.fill_intensity = 0.25f,
		.bounces = 1,
		.cast_shadows = true,
		.bounce_strength = 0.5f,
	}));
	VOE_TEST_CHECK(voe_scene_transform_add(world, cube, (voe_scene_transform){
		.position = { 0.1, -0.0, 2.0 },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	}));
	VOE_TEST_CHECK(voe_scene_camera_add(world, cube, (voe_scene_camera){
		.fov_y = 0.5f,
		.near_plane = 0.25f,
		.far_plane = 100.0f,
	}));
	return world;
}

static voe_authoring_text cooked(const voe_ecs_world *world,
				 voe_base_arena *arena, const char *include)
{
	voe_authoring_text out = { .text = "", .size = 0 };

	VOE_TEST_CHECK(voe_authoring_scene_cook(world, include, "cooked_build",
						arena, &out));
	return out;
}

static void check_refused(const voe_ecs_world *world, voe_base_arena *arena)
{
	const char *sentinel = "untouched";
	voe_authoring_text out = { .text = sentinel, .size = 12345 };

	VOE_TEST_CHECK(!voe_authoring_scene_cook(world, "game/scene.h",
						 "cooked_build", arena, &out));
	VOE_TEST_CHECK(out.text == sentinel);
	VOE_TEST_CHECK_INT(out.size, 12345);
}

static void test_cook_exact_text(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_authoring_text out = cooked(small_world(arena), arena,
					"game/scene.h");

	CHECK_TEXT(out.text, out.size,
		"#include <game/scene.h>\n"
		"\n"
		"bool cooked_build(voe_ecs_world *world)\n"
		"{\n"
		"\tvoe_ecs_entity e[2];\n"
		"\n"
		"\tfor (uint32_t i = 0; i < 2; i++)\n"
		"\t\tif (!voe_ecs_entity_create(world, &e[i]))\n"
		"\t\t\treturn false;\n"
		"\tif (!voe_ecs_component_add(world, voe_ecs_component_type(world, "
		"&voe_scene_camera_key), e[0], &(voe_scene_camera){ "
		".fov_y = 0x1p-1f, .near_plane = 0x1p-2f, .far_plane = 0x1.9p+6f }))\n"
		"\t\treturn false;\n"
		"\tif (!voe_ecs_component_add(world, voe_ecs_component_type(world, "
		"&voe_scene_identity_key), e[0], &(voe_scene_identity){ "
		".id = 3ull, .name = \"Cube\", .folded = false }))\n"
		"\t\treturn false;\n"
		"\tif (!voe_ecs_component_add(world, voe_ecs_component_type(world, "
		"&voe_scene_transform_key), e[0], &(voe_scene_transform){ "
		".position = { 0x1.999999999999ap-4, -0x0p+0, 0x1p+1 }, "
		".rotation = { 0x0p+0f, 0x0p+0f, 0x0p+0f, 0x1p+0f }, "
		".scale = { 0x1p+0f, 0x1p+0f, 0x1p+0f } }))\n"
		"\t\treturn false;\n"
		"\tif (!voe_ecs_component_add(world, voe_ecs_component_type(world, "
		"&voe_scene_identity_key), e[1], &(voe_scene_identity){ "
		".id = 7ull, .name = \"Sun \\\"key\\\"\", .folded = false }))\n"
		"\t\treturn false;\n"
		"\tif (!voe_ecs_component_add(world, voe_ecs_component_type(world, "
		"&voe_scene_light_key), e[1], &(voe_scene_light){ "
		".colour = { 0x1p+0f, 0x1p-1f, 0x1p-2f }, "
		".intensity = 0x1.8p+1f, "
		".fill_colour = { 0x1p+0f, 0x1p+0f, 0x1p+0f }, "
		".fill_intensity = 0x1p-2f, "
		".bounces = 1u, .cast_shadows = true, "
		".bounce_strength = 0x1p-1f }))\n"
		"\t\treturn false;\n"
		"\treturn true;\n"
		"}\n");
	voe_base_arena_destroy(arena);
}

static bool write_file(const char *path, const char *text, size_t size)
{
	FILE *file = fopen(path, "wb");

	if (file == NULL)
		return false;
	bool written = fwrite(text, 1, size, file) == size;
	return fclose(file) == 0 && written;
}

static void test_cook_compiles(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_authoring_text out = cooked(small_world(arena), arena,
					"cook_scene.h");
	const char *header =
		"#include <scene/camera_component.h>\n"
		"#include <scene/identity_component.h>\n"
		"#include <scene/light_component.h>\n"
		"#include <scene/transform_component.h>\n"
		"bool cooked_build(voe_ecs_world *world);\n";
	char root[1024];
	char command[4096];

	// The repository root: __FILE__ without "authoring/tests/scene_cook.c".
	size_t length = strlen(__FILE__);
	VOE_TEST_CHECK(length < sizeof(root));
	memcpy(root, __FILE__, length + 1);
	for (int up = 0; up < 3; up++) {
		while (length > 0 && root[length - 1] != '/' &&
		       root[length - 1] != '\\')
			length--;
		if (length > 0)
			length--;
	}
	root[length] = '\0';

	VOE_TEST_CHECK(write_file("cook_scene.h", header, strlen(header)));
	VOE_TEST_CHECK(write_file("cook_scene.c", out.text, out.size));
	int n = snprintf(command, sizeof(command),
			 "clang -std=c23 -fsyntax-only -Wall -Wextra -Wpedantic "
			 "-Werror -DVOE_BASE_DESCRIPTIONS=1 -I. -I%s/base/include "
			 "-I%s/math/include -I%s/ecs/include -I%s/scene/include "
			 "cook_scene.c",
			 root, root, root, root);
	VOE_TEST_CHECK(n > 0 && (size_t)n < sizeof(command));
	VOE_TEST_CHECK_INT(system(command), 0);
	voe_base_arena_destroy(arena);
}

// voe_scene_light_add asserts on a NaN colour, so the light goes in through the
// raw component add: the cook must still refuse what a world can hold.
static void test_cook_refuses_nan(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity sun = authored(world, 1, "Sun");
	const voe_scene_light light = {
		.colour = { 1.0f, NAN, 1.0f },
		.intensity = 1.0f,
	};

	VOE_TEST_CHECK(voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_light_key), sun,
		&light));
	check_refused(world, arena);
	voe_base_arena_destroy(arena);
}

static void test_cook_refuses_duplicate_id(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);

	(void)authored(world, 4, "One");
	(void)authored(world, 4, "Two");
	check_refused(world, arena);
	voe_base_arena_destroy(arena);
}

// A position 100 km out cooks to the hex literal of each of its doubles, no
// `f`, so Play starts at the same bits the editor had (ADR-0250).
static void test_cook_far_position(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = world_of(arena);
	const double far[3] = { 100000.123456789, -0.05, 1e7 + 0.001 };
	char wanted[160];

	VOE_TEST_CHECK(voe_scene_transform_add(
		world, authored(world, 5, "Far"), (voe_scene_transform){
			.position = { far[0], far[1], far[2] },
			.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
			.scale = { 1.0f, 1.0f, 1.0f },
		}));
	voe_authoring_text out = cooked(world, arena, "game/scene.h");

	(void)snprintf(wanted, sizeof(wanted), ".position = { %a, %a, %a }",
		       far[0], far[1], far[2]);
	VOE_TEST_CHECK(strstr(out.text, wanted) != NULL);
	voe_base_arena_destroy(arena);
}

static void test_cook_empty_world(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_authoring_text out = cooked(world_of(arena), arena, "game/scene.h");

	CHECK_TEXT(out.text, out.size,
		   "#include <game/scene.h>\n"
		   "\n"
		   "bool cooked_build(voe_ecs_world *world)\n"
		   "{\n"
		   "\t(void)world;\n"
		   "\treturn true;\n"
		   "}\n");
	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_cook_exact_text();
	test_cook_compiles();
	test_cook_refuses_nan();
	test_cook_refuses_duplicate_id();
	test_cook_far_position();
	test_cook_empty_world();
	return voe_test_result();
}
