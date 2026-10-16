// A Sky, Haze or Picture row whose entity is off is as no row (0440), through
// voe_3d_draw_system_sky and _picture. Needs no graphics card: neither takes a
// device. Its worlds hold the tables 3d/tests/sky.c's a_world registers for the
// sky, haze and picture, plus the identity and parent tables; an entity is
// switched as 3d/tests/lights_switched_off.c switches one.
//
// It claims six things.
// 1. A Haze row on an entity with an identity: `haze`, `haze_start` and
//    `haze_top` are the row's; the entity off: all three 0; on: the row's.
// 2. A Sky row likewise: on, `drawn` 1 and the row's colours; off, `drawn` 0
//    and the colours 0, as with no row; on, back.
// 3. A Picture row of exposure -1 and Clip, off: the zeroed exposure; on: the
//    row's.
// 4. A Sky and a Haze row on one entity, off: neither is in the record.
// 5. A Haze row on a child, its parent off: no haze; the parent on: haze.
// 6. Two Haze rows, the first's entity off: the second row's haze.
#include <3d/draw_system.h>
#include <base/arena.h>
#include <ecs/structure.h>
#include <ecs/world.h>
#include <scene/haze_component.h>
#include <scene/haze_system.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/parent_component.h>
#include <scene/parent_system.h>
#include <scene/picture_component.h>
#include <scene/picture_system.h>
#include <scene/sky_component.h>
#include <scene/sky_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#define SCRATCH (1024 * 1024)
#define ROOM 8

static const voe_scene_transform STILL = { .rotation = { 0.0f, 0.0f, 0.0f,
							 1.0f },
					   .scale = { 1.0f, 1.0f, 1.0f } };

static const voe_scene_haze HAZE = { .haze = 500.0f,
				     .start = 250.0f,
				     .height = 300.0f };

static const voe_scene_haze SECOND_HAZE = { .haze = 800.0f,
					    .start = 50.0f,
					    .height = 600.0f };

static const voe_scene_sky SKY = { .zenith = { 0.1f, 0.2f, 0.3f },
				   .horizon = { 0.4f, 0.5f, 0.6f },
				   .ground = { 0.7f, 0.8f, 0.9f },
				   .horizon_height = 0.6f,
				   .strength = 0.5f };

static const voe_math_double3 EYE = { 0.0, 40.0, 0.0 };

// A world with the sky, haze (room for two), picture, identity and parent
// tables and no row.
static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_world *world = voe_ecs_world_new(
		arena, (voe_ecs_limits){ .entities = ROOM,
					 .component_types = 16,
					 .intent_types = 16,
					 .structure_requests = 16,
					 .structure_bytes = 1024 });

	voe_scene_transform_register(world, ROOM);
	voe_scene_identity_register(world, ROOM);
	voe_scene_parent_register(world, ROOM);
	voe_scene_sky_register(world, 1);
	voe_scene_haze_register(world, 2);
	voe_scene_picture_register(world, 1);
	return world;
}

// A new placed entity with identity `id`, on.
static voe_ecs_entity an_entity(voe_ecs_world *world, uint64_t id)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, STILL));
	VOE_TEST_CHECK(voe_scene_identity_add(
		world, entity, (voe_scene_identity){ .id = id }));
	return entity;
}

// The entity's switch set to `off` through the identity intent.
static void switch_to(voe_ecs_world *world, voe_ecs_entity entity, bool off)
{
	const voe_scene_identity *read = voe_scene_identity_get(world, entity);
	voe_scene_identity row;

	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	row = *read;
	row.off = off;
	VOE_TEST_CHECK(voe_scene_identity_submit(
		world, (voe_scene_identity_intent){ .entity = entity,
						    .identity = row }));
	voe_scene_identity_system_run(world);
}

static void check_float3(voe_math_float3 got, voe_math_float3 want)
{
	VOE_TEST_CHECK_FLOAT(got.x, want.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(got.y, want.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(got.z, want.z, 0.0f);
}

// The record's haze against `haze` at EYE; a zeroed row for none.
static void check_haze(voe_render_sky sky, voe_scene_haze haze, bool none)
{
	VOE_TEST_CHECK_FLOAT(sky.haze, haze.haze, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.haze_start, haze.start, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.haze_top,
			     none ? 0.0f : (float)(haze.height - EYE.y), 0.0f);
}

// The record's sky against `row`, or with `none` every sky field nought.
static void check_sky(voe_render_sky sky, voe_scene_sky row, bool none)
{
	voe_scene_sky want = none ? (voe_scene_sky){ 0 } : row;

	VOE_TEST_CHECK_INT(sky.drawn, none ? 0 : 1);
	check_float3(sky.zenith, want.zenith);
	check_float3(sky.horizon, want.horizon);
	check_float3(sky.ground, want.ground);
	VOE_TEST_CHECK_FLOAT(sky.horizon_height, want.horizon_height, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.strength, want.strength, 0.0f);
}

static void an_off_haze_is_no_haze(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity hazed = an_entity(world, 1);

	VOE_TEST_CHECK(voe_scene_haze_add(world, hazed, HAZE));
	check_haze(voe_3d_draw_system_sky(world, EYE), HAZE, false);
	switch_to(world, hazed, true);
	check_haze(voe_3d_draw_system_sky(world, EYE), (voe_scene_haze){ 0 },
		   true);
	switch_to(world, hazed, false);
	check_haze(voe_3d_draw_system_sky(world, EYE), HAZE, false);
}

static void an_off_sky_is_no_sky(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity skied = an_entity(world, 1);

	VOE_TEST_CHECK(voe_scene_sky_add(world, skied, SKY));
	check_sky(voe_3d_draw_system_sky(world, EYE), SKY, false);
	switch_to(world, skied, true);
	check_sky(voe_3d_draw_system_sky(world, EYE), SKY, true);
	switch_to(world, skied, false);
	check_sky(voe_3d_draw_system_sky(world, EYE), SKY, false);
}

static void an_off_picture_is_x1_and_clip(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity pictured = an_entity(world, 1);
	voe_render_exposure exposure;

	VOE_TEST_CHECK(voe_scene_picture_add(
		world, pictured,
		(voe_scene_picture){ .exposure = -1.0f,
				     .tone = VOE_SCENE_PICTURE_TONE_CLIP }));
	switch_to(world, pictured, true);
	exposure = voe_3d_draw_system_picture(world);
	VOE_TEST_CHECK_FLOAT(exposure.exposure, 0.0f, 0.0f);
	VOE_TEST_CHECK_INT(exposure.tone, 0);
	switch_to(world, pictured, false);
	exposure = voe_3d_draw_system_picture(world);
	VOE_TEST_CHECK_FLOAT(exposure.exposure, -1.0f, 0.0f);
	VOE_TEST_CHECK_INT(exposure.tone, VOE_RENDER_TONE_CLIP);
}

static void one_off_entity_takes_both_rows(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity both = an_entity(world, 1);
	voe_render_sky sky;

	VOE_TEST_CHECK(voe_scene_sky_add(world, both, SKY));
	VOE_TEST_CHECK(voe_scene_haze_add(world, both, HAZE));
	switch_to(world, both, true);
	sky = voe_3d_draw_system_sky(world, EYE);
	check_sky(sky, SKY, true);
	check_haze(sky, (voe_scene_haze){ 0 }, true);
}

static void an_off_parent_takes_the_haze(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity parent = an_entity(world, 1);
	voe_ecs_entity child = an_entity(world, 2);

	VOE_TEST_CHECK(voe_scene_haze_add(world, child, HAZE));
	VOE_TEST_CHECK(voe_scene_parent_set(world, child, parent));
	voe_ecs_structure_apply(world);
	voe_scene_transform_system_run(world);
	switch_to(world, parent, true);
	check_haze(voe_3d_draw_system_sky(world, EYE), (voe_scene_haze){ 0 },
		   true);
	switch_to(world, parent, false);
	check_haze(voe_3d_draw_system_sky(world, EYE), HAZE, false);
}

static void an_off_first_haze_gives_the_second(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity first = an_entity(world, 1);
	voe_ecs_entity second = an_entity(world, 2);

	VOE_TEST_CHECK(voe_scene_haze_add(world, first, HAZE));
	VOE_TEST_CHECK(voe_scene_haze_add(world, second, SECOND_HAZE));
	VOE_TEST_CHECK_INT(voe_scene_haze_count(world), 2);
	switch_to(world, first, true);
	check_haze(voe_3d_draw_system_sky(world, EYE), SECOND_HAZE, false);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	an_off_haze_is_no_haze(arena);
	an_off_sky_is_no_sky(arena);
	an_off_picture_is_x1_and_clip(arena);
	one_off_entity_takes_both_rows(arena);
	an_off_parent_takes_the_haze(arena);
	an_off_first_haze_gives_the_second(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
