// Entities: that an id names one thing for as long as that thing is alive and
// nothing afterwards, and that a full world says so rather than handing out an
// id it cannot keep.
//
// THE ONE THAT MATTERS IS THE STALE ID. A destroyed entity's slot is handed
// straight back out, so an id kept across a destroy names a live slot holding
// something else — and a world that answered it would answer confidently with
// the wrong row. The check below destroys an entity, creates another, and
// insists the old id is refused even though the slot it names is live again.
#include <base/arena.h>
#include <ecs/world.h>

#include <testing/test.h>

// Enough for the counts below and small enough that filling it is a loop and not
// a wait.
#define ENTITIES 8

static voe_ecs_world *world_of(voe_base_arena *arena, uint32_t entities)
{
	voe_ecs_limits limits = {
		.entities = entities,
		.component_types = 2,
		.intent_types = 2,
	};

	return voe_ecs_world_new(arena, limits);
}

static void an_id_is_alive_until_it_is_destroyed(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena, ENTITIES);
	voe_ecs_entity thing = { 0 };
	voe_ecs_entity zeroed = { 0 };

	// A zeroed id is nobody's, before anything has been created and after.
	VOE_TEST_CHECK(!voe_ecs_entity_alive(world, zeroed));
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), 0);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_ecs_entity_alive(world, thing));
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), 1);
	VOE_TEST_CHECK(!voe_ecs_entity_alive(world, zeroed));

	voe_ecs_entity_destroy(world, thing);
	VOE_TEST_CHECK(!voe_ecs_entity_alive(world, thing));
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), 0);

	// Twice is not an error and does not double-count.
	voe_ecs_entity_destroy(world, thing);
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), 0);
}

static void a_stale_id_is_refused_even_though_its_slot_is_live(
	voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena, ENTITIES);
	voe_ecs_entity first = { 0 };
	voe_ecs_entity second = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &first));
	voe_ecs_entity_destroy(world, first);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &second));

	// The slot came straight back — that is what makes this worth checking.
	VOE_TEST_CHECK_INT(second.index, first.index);
	VOE_TEST_CHECK(second.generation != first.generation);

	VOE_TEST_CHECK(voe_ecs_entity_alive(world, second));
	VOE_TEST_CHECK(!voe_ecs_entity_alive(world, first));
}

static void a_full_world_says_so(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena, ENTITIES);
	voe_ecs_entity held[ENTITIES];
	voe_ecs_entity one_too_many = { 0 };

	for (uint32_t i = 0; i < ENTITIES; i++)
		VOE_TEST_CHECK(voe_ecs_entity_create(world, &held[i]));

	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), ENTITIES);
	VOE_TEST_CHECK(!voe_ecs_entity_create(world, &one_too_many));

	// And room comes back when something is destroyed, because the slot goes
	// on the free list rather than being spent.
	voe_ecs_entity_destroy(world, held[3]);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &one_too_many));
	VOE_TEST_CHECK_INT(one_too_many.index, held[3].index);
	VOE_TEST_CHECK(!voe_ecs_entity_alive(world, held[3]));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	an_id_is_alive_until_it_is_destroyed(arena);
	a_stale_id_is_refused_even_though_its_slot_is_live(arena);
	a_full_world_says_so(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
