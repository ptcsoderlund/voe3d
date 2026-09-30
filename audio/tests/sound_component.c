// The sound component: that its default row is 0304's with its menu path,
// that a row added with voe_audio_sound_add reads back through get and the
// table, that the voice type is runtime-only, and that both submits take an
// intent. Plays nothing and opens no device.
//
// THE DESCRIPTION IS SWITCHED ON HERE, WHATEVER THE BUILD SAID, for the reason
// scene/tests/identity.c gives: check.cmake builds without descriptions.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <audio/sound_component.h>

#include <base/arena.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <scene/transform_system.h>

#include <testing/test.h>

#include <string.h>

#define SCRATCH (256 * 1024)
#define ENTITIES 4

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = ENTITIES,
		.component_types = 3,
		.intent_types = 3,
		.structure_requests = 4 * ENTITIES,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	// Transforms too: a sound on a thing that has one is the usual case.
	voe_scene_transform_register(world, ENTITIES);
	voe_audio_sound_register(world, ENTITIES);
	return world;
}

static void the_default_row_is_0304s(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	const voe_ecs_type type =
		voe_ecs_component_type(world, &voe_audio_sound_key);
	const voe_audio_sound *row = voe_ecs_component_default(world, type);
	const char *menu = voe_ecs_component_menu(world, type);

	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	VOE_TEST_CHECK(row->path[0] == '\0');
	VOE_TEST_CHECK(row->playing && !row->loop);
	VOE_TEST_CHECK(row->volume == 1.0f && row->pitch == 1.0f);
	VOE_TEST_CHECK(menu != NULL && strcmp(menu, "Audio / Sound") == 0);
	voe_base_arena_clear(arena);
}

static void an_added_row_reads_back(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = { 0 };
	voe_audio_sound sound = { .loop = true, .volume = 0.5f, .pitch = 2.0f };
	const voe_audio_sound *row;

	strcpy(sound.path, "sounds/hum.wav");
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_audio_sound_add(world, entity, sound));
	row = voe_audio_sound_get(world, entity);
	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	VOE_TEST_CHECK(!row->playing && row->loop);
	VOE_TEST_CHECK(row->volume == 0.5f && row->pitch == 2.0f);
	VOE_TEST_CHECK(strcmp(row->path, "sounds/hum.wav") == 0);
	VOE_TEST_CHECK_INT(voe_audio_sound_count(world), 1);
	VOE_TEST_CHECK(voe_audio_sound_rows(world)[0].pitch == 2.0f);
	VOE_TEST_CHECK(voe_audio_sound_entities(world)[0].index == entity.index);
	VOE_TEST_CHECK_INT(voe_audio_sound_voice_count(world), 0);
	VOE_TEST_CHECK(voe_audio_sound_voice_get(world, entity) == NULL);
	VOE_TEST_CHECK(voe_ecs_component_runtime_only(
		world, voe_ecs_component_type(world, &voe_audio_sound_voice_key)));
	voe_base_arena_clear(arena);
}

static void both_submits_take_an_intent(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_audio_sound_submit(
		world, (voe_audio_sound_intent){ .entity = entity }));
	VOE_TEST_CHECK(voe_audio_sound_control_submit(
		world, (voe_audio_sound_control){ .entity = entity,
						  .kind = VOE_AUDIO_SOUND_TUNE,
						  .volume = 0.5f,
						  .pitch = 1.5f }));
	voe_base_arena_clear(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	the_default_row_is_0304s(arena);
	an_added_row_reads_back(arena);
	both_submits_take_an_intent(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
