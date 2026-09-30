// The emitter system: that a rate spawns its count over a second, a burst on a
// stopped emitter spawns at once, stop keeps the live ones and adds none, rise
// lifts, a particle dies after its life, no emitter has more than 64, and a
// removed emitter's particles row is gone after the next run. Needs no
// graphics card.
#include <3d/emitter_component.h>
#include <3d/emitter_system.h>

#include <base/arena.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <scene/transform_system.h>

#include <testing/test.h>

#define SCRATCH (256 * 1024)
#define ENTITIES 4
#define STEP (1.0f / 60.0f)

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

	voe_scene_transform_register(world, ENTITIES);
	voe_3d_emitter_register(world, ENTITIES);
	return world;
}

// An entity at the origin, unturned, carrying `emitter`.
static voe_ecs_entity an_emitter(voe_ecs_world *world, voe_3d_emitter emitter)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { .w = 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_3d_emitter_add(world, entity, emitter));
	return entity;
}

static uint32_t live(const voe_ecs_world *world, voe_ecs_entity entity)
{
	const voe_3d_particles *row = voe_3d_particles_get(world, entity);

	return row == NULL ? 0 : row->count;
}

static void a_rate_spawns_its_count(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = an_emitter(
		world, (voe_3d_emitter){ .playing = true, .rate = 20.0f,
					 .life = 10.0f, .speed = 1.0f,
					 .direction = { 0, 1, 0 } });

	for (int i = 0; i < 60; i++)
		voe_3d_emitter_system_run(world, STEP);
	VOE_TEST_CHECK(live(world, entity) >= 19 && live(world, entity) <= 21);
	voe_base_arena_clear(arena);
}

static void a_burst_on_a_stopped_emitter(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = an_emitter(
		world, (voe_3d_emitter){ .rate = 20.0f, .life = 10.0f });

	VOE_TEST_CHECK(voe_3d_emitter_control_submit(
		world, (voe_3d_emitter_control){ .entity = entity,
						 .kind = VOE_3D_EMITTER_BURST,
						 .count = 10 }));
	voe_3d_emitter_system_run(world, STEP);
	VOE_TEST_CHECK_INT(live(world, entity), 10);
	voe_base_arena_clear(arena);
}

static void stop_keeps_the_live_ones(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = an_emitter(
		world, (voe_3d_emitter){ .playing = true, .rate = 60.0f,
					 .life = 10.0f });
	uint32_t before;

	for (int i = 0; i < 10; i++)
		voe_3d_emitter_system_run(world, STEP);
	before = live(world, entity);
	VOE_TEST_CHECK(before > 0);
	VOE_TEST_CHECK(voe_3d_emitter_control_submit(
		world, (voe_3d_emitter_control){ .entity = entity,
						 .kind = VOE_3D_EMITTER_STOP }));
	for (int i = 0; i < 10; i++)
		voe_3d_emitter_system_run(world, STEP);
	VOE_TEST_CHECK_INT(live(world, entity), before);
	VOE_TEST_CHECK(!voe_3d_emitter_get(world, entity)->playing);
	voe_base_arena_clear(arena);
}

static void rise_lifts(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = an_emitter(
		world, (voe_3d_emitter){ .burst = 1, .life = 10.0f,
					 .rise = 1.0f });
	const voe_3d_particles *row;

	VOE_TEST_CHECK(voe_3d_emitter_control_submit(
		world, (voe_3d_emitter_control){ .entity = entity,
						 .kind = VOE_3D_EMITTER_BURST }));
	voe_3d_emitter_system_run(world, STEP);
	row = voe_3d_particles_get(world, entity);
	VOE_TEST_CHECK(row != NULL && row->count == 1);
	if (row != NULL && row->count == 1)
		VOE_TEST_CHECK(row->particles[0].position.y > 0.0);
	voe_base_arena_clear(arena);
}

static void a_particle_dies_after_its_life(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = an_emitter(
		world, (voe_3d_emitter){ .life = 0.5f, .speed = 1.0f });

	VOE_TEST_CHECK(voe_3d_emitter_control_submit(
		world, (voe_3d_emitter_control){ .entity = entity,
						 .kind = VOE_3D_EMITTER_BURST,
						 .count = 3 }));
	voe_3d_emitter_system_run(world, STEP);
	VOE_TEST_CHECK_INT(live(world, entity), 3);
	for (int i = 0; i < 31; i++)
		voe_3d_emitter_system_run(world, STEP);
	VOE_TEST_CHECK_INT(live(world, entity), 0);
	voe_base_arena_clear(arena);
}

static void never_more_than_64(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = an_emitter(
		world, (voe_3d_emitter){ .playing = true, .rate = 10000.0f,
					 .burst = 500, .life = 10.0f });
	bool within = true;

	VOE_TEST_CHECK(voe_3d_emitter_control_submit(
		world, (voe_3d_emitter_control){ .entity = entity,
						 .kind = VOE_3D_EMITTER_PLAY }));
	for (int i = 0; i < 30; i++) {
		voe_3d_emitter_system_run(world, STEP);
		within = within && live(world, entity) <= VOE_3D_EMITTER_PARTICLES;
	}
	VOE_TEST_CHECK(within);
	VOE_TEST_CHECK_INT(live(world, entity), VOE_3D_EMITTER_PARTICLES);
	voe_base_arena_clear(arena);
}

static void a_removed_emitters_row_goes(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = an_emitter(
		world, (voe_3d_emitter){ .playing = true, .rate = 20.0f,
					 .life = 1.0f });

	voe_3d_emitter_system_run(world, STEP);
	VOE_TEST_CHECK(voe_3d_particles_get(world, entity) != NULL);
	VOE_TEST_CHECK(voe_ecs_component_remove(
		world, voe_ecs_component_type(world, &voe_3d_emitter_key),
		entity));
	voe_3d_emitter_system_run(world, STEP);
	VOE_TEST_CHECK(voe_3d_particles_get(world, entity) == NULL);
	VOE_TEST_CHECK_INT(voe_3d_particles_count(world), 0);
	voe_base_arena_clear(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	a_rate_spawns_its_count(arena);
	a_burst_on_a_stopped_emitter(arena);
	stop_keeps_the_live_ones(arena);
	rise_lifts(arena);
	a_particle_dies_after_its_life(arena);
	never_more_than_64(arena);
	a_removed_emitters_row_goes(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
