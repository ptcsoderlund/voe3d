// The reference text (authoring/reference.h): a world with scene's transform,
// identity and light registered gives `## voe_scene_light` with rows for
// colour, intensity and disc_size, the disc's row showing 0, 10 and °, and no
// key the light does not have; material's key table gives emission_strength's
// greatest, 1000, and repeat.
//
// The world is set up as scene_check.c's is.
#include <authoring/reference.h>

#include <assets/material.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <scene/identity_system.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdint.h>
#include <string.h>

#define ENTITIES 8

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

// The table row that starts with `start`, up to its newline, copied into `row`.
static bool row_of(const char *text, const char *start, char *row, size_t size)
{
	const char *found = strstr(text, start);

	if (found == NULL)
		return false;

	const char *end = strchr(found, '\n');
	size_t length = end != NULL ? (size_t)(end - found) : strlen(found);

	if (length >= size)
		length = size - 1;
	memcpy(row, found, length);
	row[length] = '\0';
	return true;
}

static void test_reference_scene_light(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	voe_ecs_world *world = world_of(arena);
	voe_authoring_text out = { 0 };
	char row[512];

	VOE_TEST_CHECK(voe_authoring_reference_scene(world, arena, &out));
	VOE_TEST_CHECK(out.text != NULL && strlen(out.text) == out.size);
	VOE_TEST_CHECK(strstr(out.text, "## voe_scene_light\n") != NULL);
	VOE_TEST_CHECK(strstr(out.text, "| `colour` |") != NULL);
	VOE_TEST_CHECK(strstr(out.text, "| `intensity` |") != NULL);
	VOE_TEST_CHECK(row_of(out.text, "| `disc_size` |", row, sizeof(row)));
	VOE_TEST_CHECK(strstr(row, "0") != NULL);
	VOE_TEST_CHECK(strstr(row, "10") != NULL);
	VOE_TEST_CHECK(strstr(row, "°") != NULL);
	VOE_TEST_CHECK(strstr(out.text, "texture") == NULL);
	voe_base_arena_destroy(arena);
}

static void test_reference_keys_material(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	uint32_t count = 0;
	const voe_assets_documented_key *keys = voe_assets_material_keys(&count);
	voe_authoring_text out = { 0 };
	char row[512];

	VOE_TEST_CHECK(voe_authoring_reference_keys("Material files", keys,
						    count, arena, &out));
	VOE_TEST_CHECK(out.text != NULL && strlen(out.text) == out.size);
	VOE_TEST_CHECK(row_of(out.text, "| `[Material]` | `emission_strength` |",
			      row, sizeof(row)));
	VOE_TEST_CHECK(strstr(row, "1000") != NULL);
	VOE_TEST_CHECK(strstr(out.text, "`repeat`") != NULL);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_reference_scene_light();
	test_reference_keys_material();
	return voe_test_result();
}
