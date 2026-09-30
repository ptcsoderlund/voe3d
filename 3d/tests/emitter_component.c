// The emitter component: that a row added with voe_3d_emitter_add reads back
// its fields, that the default row is 0298's, that the particles type is
// runtime-only, and that both submits take an intent. Needs no graphics card.
//
// THE DESCRIPTION IS SWITCHED ON HERE, WHATEVER THE BUILD SAID, for the reason
// scene/tests/identity.c gives: check.cmake builds without descriptions.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <3d/emitter_component.h>

#include <base/arena.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <scene/transform_component.h>
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

	// Transform first: an emitter's registration names what it needs.
	voe_scene_transform_register(world, ENTITIES);
	voe_3d_emitter_register(world, ENTITIES);
	return world;
}

static void an_added_row_reads_back(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = { 0 };
	voe_3d_emitter emitter = { .rate = 5.0f, .burst = 12, .life = 3.0f,
				   .spread = 45.0f, .direction = { 1, 0, 0 },
				   .colour_end = { 1, 0.5f, 0 }, .glow = 2.0f };
	const voe_3d_emitter *row;

	strcpy(emitter.texture, "Assets/smoke.png");
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_3d_emitter_add(world, entity, emitter));
	row = voe_3d_emitter_get(world, entity);
	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	VOE_TEST_CHECK(!row->playing && row->rate == 5.0f && row->life == 3.0f);
	VOE_TEST_CHECK(row->spread == 45.0f && row->direction.x == 1);
	VOE_TEST_CHECK(row->colour_end.y == 0.5f && row->glow == 2.0f);
	VOE_TEST_CHECK_INT(voe_3d_emitter_count(world), 1);
	VOE_TEST_CHECK(voe_3d_emitter_rows(world)[0].burst == 12);
	VOE_TEST_CHECK(voe_3d_emitter_entities(world)[0].index == entity.index);
	VOE_TEST_CHECK(strcmp(row->texture, "Assets/smoke.png") == 0);
	VOE_TEST_CHECK_INT(voe_3d_particles_count(world), 0);
	VOE_TEST_CHECK(voe_3d_particles_get(world, entity) == NULL);
	VOE_TEST_CHECK(voe_ecs_component_runtime_only(
		world, voe_ecs_component_type(world, &voe_3d_particles_key)));
	voe_base_arena_clear(arena);
}

static void the_default_row_is_0298s(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	const voe_3d_emitter *row = voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_3d_emitter_key));

	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	VOE_TEST_CHECK(row->playing);
	VOE_TEST_CHECK(row->rate == 20.0f && row->burst == 0);
	VOE_TEST_CHECK(row->life == 1.5f && row->speed == 1.0f);
	VOE_TEST_CHECK(row->spread == 20.0f);
	VOE_TEST_CHECK(row->offset.x == 0 && row->offset.y == 0 &&
		       row->offset.z == 0);
	VOE_TEST_CHECK(row->direction.x == 0 && row->direction.y == 1 &&
		       row->direction.z == 0);
	VOE_TEST_CHECK(row->rise == 0 && row->drag == 0);
	VOE_TEST_CHECK(row->size_start == 0.3f && row->size_end == 0.6f);
	VOE_TEST_CHECK(row->colour_start.x == 1 && row->colour_start.y == 1 &&
		       row->colour_start.z == 1);
	VOE_TEST_CHECK(row->colour_end.x == 1 && row->colour_end.y == 1 &&
		       row->colour_end.z == 1);
	VOE_TEST_CHECK(row->alpha_start == 1 && row->alpha_end == 0);
	VOE_TEST_CHECK(row->glow == 0 && row->texture[0] == '\0');
	voe_base_arena_clear(arena);
}

static void both_submits_take_an_intent(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_3d_emitter_submit(
		world, (voe_3d_emitter_intent){ .entity = entity }));
	VOE_TEST_CHECK(voe_3d_emitter_control_submit(
		world, (voe_3d_emitter_control){ .entity = entity,
						 .kind = VOE_3D_EMITTER_BURST,
						 .count = 8 }));
	voe_base_arena_clear(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	an_added_row_reads_back(arena);
	the_default_row_is_0298s(arena);
	both_submits_take_an_intent(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
