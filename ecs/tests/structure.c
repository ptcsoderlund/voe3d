// The structural queue: that nothing changes until _apply and then exactly what
// was asked for, with the bytes given; that requests apply in the order they
// were submitted; that every drop case is dropped without asserting; that a full
// queue — out of requests, or out of bytes — refuses the submit; and that a
// world made without a queue refuses every one.
//
// THE ORDER CHECKS ARE THE ONES A SORTED OR BATCHED QUEUE WOULD FAIL. An add then
// a remove of the same row, and a destroy then an add to the same entity, both
// leave nothing only when the requests run in the order they came in.
#include <base/arena.h>
#include <ecs/component.h>
#include <ecs/structure.h>
#include <ecs/world.h>

#include <testing/test.h>

#include <string.h>

#define ENTITIES 8
#define ROWS 2
#define REQUESTS 4

// Four bytes each, so a byte limit is easy to count in rows.
struct marker {
	uint32_t tag;
};

static const struct voe_ecs_key marker_key = { "test_marker" };

static voe_ecs_world *world_with(voe_base_arena *arena, uint32_t requests,
				 uint32_t bytes, voe_ecs_type *type)
{
	voe_ecs_world *world = voe_ecs_world_new(arena, (voe_ecs_limits){
		.entities = ENTITIES,
		.component_types = 1,
		.intent_types = 1,
		.structure_requests = requests,
		.structure_bytes = bytes,
	});

	*type = voe_ecs_component_register(world, &marker_key,
					   sizeof(struct marker), ROWS,
					   &voe_ecs_runtime_only);
	return world;
}

static voe_ecs_world *world_of(voe_base_arena *arena, voe_ecs_type *type)
{
	return world_with(arena, REQUESTS,
			  REQUESTS * (uint32_t)sizeof(struct marker), type);
}

static voe_ecs_entity made(voe_ecs_world *world)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	return entity;
}

static void an_add_lands_at_apply_with_its_bytes(voe_base_arena *arena)
{
	voe_ecs_type type;
	voe_ecs_world *world = world_of(arena, &type);
	voe_ecs_entity thing = made(world);
	static const unsigned char given[sizeof(struct marker)] = { 0x11, 0x22,
								    0x33, 0x44 };
	const unsigned char *read;

	VOE_TEST_CHECK(voe_ecs_structure_add(world, type, thing, given));
	VOE_TEST_CHECK_INT(voe_ecs_structure_count(world), 1);
	VOE_TEST_CHECK(voe_ecs_component_get(world, type, thing) == NULL);

	voe_ecs_structure_apply(world);
	VOE_TEST_CHECK_INT(voe_ecs_structure_count(world), 0);
	read = voe_ecs_component_get(world, type, thing);
	VOE_TEST_CHECK(read != NULL);
	if (read != NULL)
		VOE_TEST_CHECK(memcmp(read, given, sizeof given) == 0);
}

static void a_remove_and_a_destroy_land_at_apply(voe_base_arena *arena)
{
	voe_ecs_type type;
	voe_ecs_world *world = world_of(arena, &type);
	voe_ecs_entity kept = made(world);
	voe_ecs_entity gone = made(world);
	struct marker value = { .tag = 1 };

	VOE_TEST_CHECK(voe_ecs_component_add(world, type, kept, &value));
	VOE_TEST_CHECK(voe_ecs_structure_remove(world, type, kept));
	VOE_TEST_CHECK(voe_ecs_structure_destroy(world, gone));
	VOE_TEST_CHECK(voe_ecs_component_get(world, type, kept) != NULL);
	VOE_TEST_CHECK(voe_ecs_entity_alive(world, gone));

	voe_ecs_structure_apply(world);
	VOE_TEST_CHECK(voe_ecs_component_get(world, type, kept) == NULL);
	VOE_TEST_CHECK(voe_ecs_entity_alive(world, kept));
	VOE_TEST_CHECK(!voe_ecs_entity_alive(world, gone));
	VOE_TEST_CHECK_INT(voe_ecs_structure_count(world), 0);
}

static void requests_apply_in_submission_order(voe_base_arena *arena)
{
	voe_ecs_type type;
	voe_ecs_world *world = world_of(arena, &type);
	voe_ecs_entity added_then_removed = made(world);
	voe_ecs_entity destroyed_then_added = made(world);
	struct marker value = { .tag = 3 };

	VOE_TEST_CHECK(voe_ecs_structure_add(world, type, added_then_removed,
					     &value));
	VOE_TEST_CHECK(voe_ecs_structure_remove(world, type,
						added_then_removed));
	VOE_TEST_CHECK(voe_ecs_structure_destroy(world, destroyed_then_added));
	VOE_TEST_CHECK(voe_ecs_structure_add(world, type, destroyed_then_added,
					     &value));

	voe_ecs_structure_apply(world);
	VOE_TEST_CHECK(voe_ecs_component_get(world, type, added_then_removed) ==
		       NULL);
	VOE_TEST_CHECK(!voe_ecs_entity_alive(world, destroyed_then_added));
	VOE_TEST_CHECK_INT(voe_ecs_component_count(world, type), 0);
	VOE_TEST_CHECK_INT(voe_ecs_structure_count(world), 0);
}

// Each of these would be false from _add or _remove, or an assert from _add in
// the already-has case; here every one is dropped and the program goes on.
static void every_drop_case_is_dropped(voe_base_arena *arena)
{
	voe_ecs_type type;
	voe_ecs_world *world = world_with(arena, 8,
					  8 * (uint32_t)sizeof(struct marker),
					  &type);
	voe_ecs_entity dead = made(world);
	voe_ecs_entity has_one = made(world);
	voe_ecs_entity filler = made(world);
	voe_ecs_entity no_room = made(world);
	struct marker first = { .tag = 1 };
	struct marker second = { .tag = 2 };
	const struct marker *read;

	voe_ecs_entity_destroy(world, dead);
	VOE_TEST_CHECK(voe_ecs_component_add(world, type, has_one, &first));
	VOE_TEST_CHECK(voe_ecs_component_add(world, type, filler, &first));

	VOE_TEST_CHECK(voe_ecs_structure_add(world, type, dead, &second));
	VOE_TEST_CHECK(voe_ecs_structure_add(world, type, has_one, &second));
	VOE_TEST_CHECK(voe_ecs_structure_add(world, type, no_room, &second));
	VOE_TEST_CHECK(voe_ecs_structure_remove(world, type, no_room));
	VOE_TEST_CHECK(voe_ecs_structure_destroy(world, dead));

	voe_ecs_structure_apply(world);
	VOE_TEST_CHECK_INT(voe_ecs_structure_count(world), 0);
	VOE_TEST_CHECK_INT(voe_ecs_component_count(world, type), ROWS);
	VOE_TEST_CHECK(voe_ecs_component_get(world, type, no_room) == NULL);
	read = voe_ecs_component_get(world, type, has_one);
	VOE_TEST_CHECK(read != NULL);
	if (read != NULL)
		VOE_TEST_CHECK_INT(read->tag, 1);
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), 3);
}

static void a_queue_out_of_requests_refuses(voe_base_arena *arena)
{
	voe_ecs_type type;
	voe_ecs_world *world = world_of(arena, &type);
	voe_ecs_entity thing = made(world);

	for (uint32_t i = 0; i < REQUESTS; i++)
		VOE_TEST_CHECK(voe_ecs_structure_remove(world, type, thing));

	VOE_TEST_CHECK(!voe_ecs_structure_remove(world, type, thing));
	VOE_TEST_CHECK(!voe_ecs_structure_destroy(world, thing));
	VOE_TEST_CHECK_INT(voe_ecs_structure_count(world), REQUESTS);

	voe_ecs_structure_apply(world);
	VOE_TEST_CHECK_INT(voe_ecs_structure_count(world), 0);
}

// Room for many requests and one row's bytes: the second add is refused while a
// remove, which carries none, still fits.
static void a_queue_out_of_bytes_refuses_an_add(voe_base_arena *arena)
{
	voe_ecs_type type;
	voe_ecs_world *world = world_with(arena, REQUESTS,
					  (uint32_t)sizeof(struct marker),
					  &type);
	voe_ecs_entity thing = made(world);
	struct marker value = { .tag = 5 };

	VOE_TEST_CHECK(voe_ecs_structure_add(world, type, thing, &value));
	VOE_TEST_CHECK(!voe_ecs_structure_add(world, type, thing, &value));
	VOE_TEST_CHECK(voe_ecs_structure_remove(world, type, thing));
	VOE_TEST_CHECK_INT(voe_ecs_structure_count(world), 2);

	// And applying gives the bytes back.
	voe_ecs_structure_apply(world);
	VOE_TEST_CHECK(voe_ecs_structure_add(world, type, thing, &value));
}

static void a_world_without_a_queue_refuses_everything(voe_base_arena *arena)
{
	voe_ecs_type type;
	voe_ecs_world *world = world_with(arena, 0, 0, &type);
	voe_ecs_entity thing = made(world);
	struct marker value = { .tag = 1 };

	VOE_TEST_CHECK(!voe_ecs_structure_add(world, type, thing, &value));
	VOE_TEST_CHECK(!voe_ecs_structure_remove(world, type, thing));
	VOE_TEST_CHECK(!voe_ecs_structure_destroy(world, thing));
	VOE_TEST_CHECK_INT(voe_ecs_structure_count(world), 0);

	voe_ecs_structure_apply(world);
	VOE_TEST_CHECK(voe_ecs_entity_alive(world, thing));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	an_add_lands_at_apply_with_its_bytes(arena);
	a_remove_and_a_destroy_land_at_apply(arena);
	requests_apply_in_submission_order(arena);
	every_drop_case_is_dropped(arena);
	a_queue_out_of_requests_refuses(arena);
	a_queue_out_of_bytes_refuses_an_add(arena);
	a_world_without_a_queue_refuses_everything(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
