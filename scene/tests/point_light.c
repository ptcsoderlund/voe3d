// The point light: what registration tells a tool, that a refused replace keeps
// the row, and the strength a reader draws with — steady, flashed, fading,
// flashed when made, re-flashed by a replace, and untouched by a flash it
// cannot take.
//
// EACH REFUSAL IS ITS OWN INTENT, submitted one at a time with the row checked
// after each, so a refusal that let one field through shows as that field.
//
// THE NUMBERS ARE CHOSEN TO BE EXACT: an intensity of 2 over a flash of 2 s,
// counted down by whole and half seconds, so the strength is compared with no
// tolerance and a fade that is off by one run shows as a wrong number.
#include <base/arena.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/point_light_component.h>
#include <scene/point_light_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <string.h>

#define LIGHTS 4

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = LIGHTS,
		.component_types = 3,
		.intent_types = 3,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, LIGHTS);
	voe_scene_point_light_register(world, LIGHTS);
	return world;
}

// No field white, one or nought, so a field copied from the wrong place shows.
static voe_scene_point_light steady(void)
{
	return (voe_scene_point_light){
		.colour = { 1.0f, 0.8f, 0.5f },
		.intensity = 2.0f,
		.range = 3.0f,
	};
}

static voe_scene_point_light flashing(void)
{
	voe_scene_point_light light = steady();

	light.flash = 2.0f;
	return light;
}

static void check_light(const voe_scene_point_light *read,
			voe_scene_point_light expected)
{
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(read->colour.x, expected.colour.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->colour.y, expected.colour.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->colour.z, expected.colour.z, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->intensity, expected.intensity, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->range, expected.range, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->flash, expected.flash, 0.0f);
	VOE_TEST_CHECK(read->flash_when_made == expected.flash_when_made);
}

static voe_ecs_entity lamp_of(voe_ecs_world *world, voe_scene_point_light light)
{
	voe_ecs_entity lamp = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &lamp));
	VOE_TEST_CHECK(voe_scene_point_light_add(world, lamp, light));
	return lamp;
}

// The replace, the default row (white, 1, 5 m, steady), the transform it needs,
// the menu path, and the glow table runtime-only while the light is not.
static void registration_says_what_a_point_light_is(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type =
		voe_ecs_component_type(world, &voe_scene_point_light_key);
	voe_ecs_type glows =
		voe_ecs_component_type(world, &voe_scene_point_light_glow_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);
	voe_ecs_type needed = { 0 };

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT((long long)replace.row_offset,
			   (long long)offsetof(voe_scene_point_light_intent,
					       light));
	check_light(voe_ecs_component_default(world, type),
		    (voe_scene_point_light){ .colour = { 1.0f, 1.0f, 1.0f },
					     .intensity = 1.0f,
					     .range = 5.0f });
	VOE_TEST_CHECK(voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK(voe_ecs_component_key(world, needed) ==
		       &voe_scene_transform_key);
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Rendering / Point light") == 0);
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, type));
	VOE_TEST_CHECK(voe_ecs_component_runtime_only(world, glows));
}

// Each bad number on its own: the row stays what it was.
static void a_refused_replace_keeps_the_row(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity lamp = lamp_of(world, flashing());
	voe_scene_point_light bad[11];

	for (int i = 0; i < 11; i++)
		bad[i] = flashing();
	bad[0].colour.x = 1.5f;
	bad[1].colour.y = -0.1f;
	bad[2].colour.z = NAN;
	bad[3].intensity = -1.0f;
	bad[4].intensity = INFINITY;
	bad[5].range = 0.0f;
	bad[6].range = -1.0f;
	bad[7].range = NAN;
	bad[8].flash = -0.5f;
	bad[9].flash = INFINITY;
	bad[10].flash = NAN;

	for (int i = 0; i < 11; i++) {
		VOE_TEST_CHECK(voe_scene_point_light_submit(
			world, (voe_scene_point_light_intent){
				       .entity = lamp, .light = bad[i] }));
		voe_scene_point_light_system_run(world, 0.0f);
		check_light(voe_scene_point_light_get(world, lamp), flashing());
		// Refused, so not re-flashed either.
		VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp),
				     0.0f, 0.0f);
	}
}

static void a_steady_light_is_its_intensity(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity lamp = lamp_of(world, steady());

	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 2.0f,
			     0.0f);
	voe_scene_point_light_system_run(world, 1.0f);
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 2.0f,
			     0.0f);
}

// Dark until flashed, full after the run that flashes it, half after half its
// flash, then nought and never below.
static void a_flash_fades_from_full_to_dark(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity lamp = lamp_of(world, flashing());

	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 0.0f,
			     0.0f);
	voe_scene_point_light_system_run(world, 0.5f);
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 0.0f,
			     0.0f);

	VOE_TEST_CHECK(voe_scene_point_light_flash_submit(world, lamp));
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 0.0f,
			     0.0f);
	voe_scene_point_light_system_run(world, 0.5f);
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 2.0f,
			     0.0f);
	voe_scene_point_light_system_run(world, 1.0f);
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 1.0f,
			     0.0f);
	voe_scene_point_light_system_run(world, 1.0f);
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 0.0f,
			     0.0f);
	voe_scene_point_light_system_run(world, 1.0f);
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 0.0f,
			     0.0f);
}

static void flash_when_made_is_full_on_its_first_run(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_scene_point_light light = flashing();
	voe_ecs_entity lamp;

	light.flash_when_made = true;
	lamp = lamp_of(world, light);
	voe_scene_point_light_system_run(world, 0.5f);
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 2.0f,
			     0.0f);
	voe_scene_point_light_system_run(world, 1.0f);
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 1.0f,
			     0.0f);
}

// A faded light given a replace lands full, at the new intensity.
static void a_replace_flashes_again(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity lamp = lamp_of(world, flashing());
	voe_scene_point_light brighter = flashing();

	VOE_TEST_CHECK(voe_scene_point_light_flash_submit(world, lamp));
	voe_scene_point_light_system_run(world, 0.0f);
	voe_scene_point_light_system_run(world, 2.0f);
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 0.0f,
			     0.0f);

	brighter.intensity = 4.0f;
	VOE_TEST_CHECK(voe_scene_point_light_submit(
		world, (voe_scene_point_light_intent){ .entity = lamp,
						       .light = brighter }));
	voe_scene_point_light_system_run(world, 1.0f);
	check_light(voe_scene_point_light_get(world, lamp), brighter);
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 4.0f,
			     0.0f);
}

// A steady light's glow stays dark, and an entity with no light gets no glow.
static void a_flash_it_cannot_take_changes_nothing(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type glows =
		voe_ecs_component_type(world, &voe_scene_point_light_glow_key);
	voe_ecs_entity lamp = lamp_of(world, steady());
	voe_ecs_entity bare = { 0 };
	const voe_scene_point_light_glow *glow;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &bare));
	VOE_TEST_CHECK(voe_scene_point_light_flash_submit(world, lamp));
	VOE_TEST_CHECK(voe_scene_point_light_flash_submit(world, bare));
	voe_scene_point_light_system_run(world, 0.0f);

	check_light(voe_scene_point_light_get(world, lamp), steady());
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, lamp), 2.0f,
			     0.0f);
	glow = voe_ecs_component_get(world, glows, lamp);
	VOE_TEST_CHECK(glow != NULL);
	if (glow != NULL)
		VOE_TEST_CHECK_FLOAT(glow->left, 0.0f, 0.0f);

	VOE_TEST_CHECK(voe_scene_point_light_get(world, bare) == NULL);
	VOE_TEST_CHECK(voe_ecs_component_get(world, glows, bare) == NULL);
	VOE_TEST_CHECK_INT(voe_ecs_component_count(world, glows), 1);
	VOE_TEST_CHECK_FLOAT(voe_scene_point_light_strength(world, bare), 0.0f,
			     0.0f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	registration_says_what_a_point_light_is(arena);
	a_refused_replace_keeps_the_row(arena);
	a_steady_light_is_its_intensity(arena);
	a_flash_fades_from_full_to_dark(arena);
	flash_when_made_is_full_on_its_first_run(arena);
	a_replace_flashes_again(arena);
	a_flash_it_cannot_take_changes_nothing(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
