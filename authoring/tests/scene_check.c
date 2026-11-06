// The strict check (voe_authoring_scene_check, 0509 point 3): a key naming no
// field of a sun's light is refused and the kept report names it and its line,
// a former name (0353) passes, a disc_size past VOE_SCENE_LIGHT_DISC_SIZE_MAX is
// refused, a section of a type nothing registered passes, a value of the wrong
// kind is refused as the reader refuses it, the text scene_write makes of a
// world with a sun passes, and a check creates no entity.
//
// The world is set up as scene_lights.c's is, with scene's real transform,
// identity and light registered, and its light blocker for the former name.
// Fields a section does not say and each refusal print to stderr; that is the
// report doing its job, not a failure.
#include <authoring/scene_read.h>
#include <authoring/scene_write.h>

#include <base/arena.h>
#include <base/report.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/light_blocker_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdint.h>
#include <stdio.h>
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
	voe_scene_light_blocker_register(world, ENTITIES);
	return world;
}

// Checks `text` against a fresh world, the kept error cleared first.
static bool check(const char *text)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	voe_ecs_world *world = world_of(arena);

	voe_base_report_error_clear();

	bool passed = voe_authoring_scene_check(text, strlen(text), world, arena);

	voe_base_arena_destroy(arena);
	return passed;
}

// The kept report holds `part`.
static void check_report_names(const char *part)
{
	const char *report = voe_base_report_error_first();

	VOE_TEST_CHECK(report != NULL && strstr(report, part) != NULL);
}

// A sun's light section holding `line`, which is line 6.
static bool check_sun_with(const char *line)
{
	char text[512];

	snprintf(text, sizeof(text),
		 "[1]\n"
		 "name = \"Sun\"\n"
		 "folded = false\n"
		 "[1.voe_scene_light]\n"
		 "colour = [1, 1, 1]\n"
		 "%s\n",
		 line);
	return check(text);
}

static void test_an_unknown_key_is_refused_naming_it(void)
{
	VOE_TEST_CHECK(!check_sun_with("texture = foo.png"));
	check_report_names("texture");
	check_report_names("line 6");
}

static void test_a_former_name_passes(void)
{
	VOE_TEST_CHECK(check("[1]\n"
			     "name = \"Wall\"\n"
			     "folded = false\n"
			     "[1.voe_scene_light_blocker]\n"
			     "size = [1, 1, 1]\n"
			     "kind = 1\n"));
}

static void test_a_value_out_of_range_is_refused(void)
{
	char line[64];

	snprintf(line, sizeof(line), "disc_size = %g",
		 (double)VOE_SCENE_LIGHT_DISC_SIZE_MAX + 1.0);
	VOE_TEST_CHECK(!check_sun_with(line));
	check_report_names("disc_size");
	check_report_names("°");
	VOE_TEST_CHECK(check_sun_with("disc_size = 10"));
}

static void test_an_unregistered_section_passes(void)
{
	VOE_TEST_CHECK(check("[1]\n"
			     "name = \"Thing\"\n"
			     "folded = false\n"
			     "[1.another_programs_component]\n"
			     "texture = \"foo.png\"\n"));
}

static void test_what_the_reader_refuses_is_refused(void)
{
	VOE_TEST_CHECK(!check_sun_with("intensity = bright"));
	check_report_names("intensity");
}

// A world holding one sun, turned and lit.
static voe_ecs_world *world_with_sun(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity entity = { 0 };
	voe_scene_identity identity = { .id = 1 };

	memcpy(identity.name, "Sun", 3);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_identity_add(world, entity, identity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, (voe_scene_transform){
		.rotation = { -0.38268343f, 0.0f, 0.0f, 0.9238795f },
		.scale = { 1.0f, 1.0f, 1.0f },
	}));
	VOE_TEST_CHECK(voe_scene_light_add(world, entity, (voe_scene_light){
		.colour = { 1.0f, 1.0f, 1.0f },
		.intensity = 3.0f,
		.bounces = 1,
		.cast_shadows = true,
		.bounce_strength = 1.0f,
		.disc_size = 2.0f,
		.glow_size = 0.5f,
	}));
	return world;
}

static void test_a_written_scene_passes(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	voe_ecs_world *world = world_with_sun(arena);
	voe_authoring_text out = { 0 };

	VOE_TEST_CHECK(voe_authoring_scene_write(world, NULL, arena, &out));
	VOE_TEST_CHECK(out.text != NULL &&
		       voe_authoring_scene_check(out.text, out.size,
						 world_of(arena), arena));
	voe_base_arena_destroy(arena);
}

static void test_nothing_is_created(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	voe_ecs_world *world = world_with_sun(arena);
	voe_authoring_text out = { 0 };

	VOE_TEST_CHECK(voe_authoring_scene_write(world, NULL, arena, &out));
	VOE_TEST_CHECK(out.text != NULL &&
		       voe_authoring_scene_check(out.text, out.size, world,
						 arena));
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), 1);
	VOE_TEST_CHECK_INT(voe_scene_light_count(world), 1);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_an_unknown_key_is_refused_naming_it();
	test_a_former_name_passes();
	test_a_value_out_of_range_is_refused();
	test_an_unregistered_section_passes();
	test_what_the_reader_refuses_is_refused();
	test_a_written_scene_passes();
	test_nothing_is_created();
	return voe_test_result();
}
