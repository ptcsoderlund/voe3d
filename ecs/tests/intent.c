// Intent queues: that two submitters land in one queue in the order they
// submitted, that reading does not empty it and clearing does, and that a full
// queue is a returned failure.
//
// SUBMISSION ORDER IS THE CLAIM WORTH CHECKING. Two places submit, alternating,
// and the owner has to see the values in the order they were handed over —
// because that is the whole of what an intent queue promises about ordering, and
// a queue built on anything but an append would quietly promise something else.
#include <base/arena.h>
#include <ecs/intent.h>
#include <ecs/world.h>

#include <testing/test.h>

#define QUEUED 6

struct nudge {
	uint32_t from;
	uint32_t value;
};

static const struct voe_ecs_key nudge_key = { "test_nudge" };

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 4,
		.component_types = 1,
		.intent_types = 2,
	};

	return voe_ecs_world_new(arena, limits);
}

// Two submitters, which in the engine would be two folders that know nothing
// about each other.
static void from_here(voe_ecs_world *world, voe_ecs_intent type, uint32_t value)
{
	struct nudge nudge = { .from = 1, .value = value };

	VOE_TEST_CHECK(voe_ecs_intent_submit(world, type, &nudge));
}

static void from_there(voe_ecs_world *world, voe_ecs_intent type, uint32_t value)
{
	struct nudge nudge = { .from = 2, .value = value };

	VOE_TEST_CHECK(voe_ecs_intent_submit(world, type, &nudge));
}

static void two_submitters_arrive_in_submission_order(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_intent type = voe_ecs_intent_register(world, &nudge_key,
						      sizeof(struct nudge),
						      QUEUED);
	const struct nudge *queued;

	VOE_TEST_CHECK_INT(voe_ecs_intent_type(world, &nudge_key).value,
			   type.value);
	VOE_TEST_CHECK_INT(voe_ecs_intent_count(world, type), 0);

	from_here(world, type, 10);
	from_there(world, type, 20);
	from_here(world, type, 30);
	from_there(world, type, 40);

	VOE_TEST_CHECK_INT(voe_ecs_intent_count(world, type), 4);

	// The drain, as the owning system would do it.
	queued = voe_ecs_intent_queue(world, type);
	VOE_TEST_CHECK_INT(queued[0].from, 1);
	VOE_TEST_CHECK_INT(queued[0].value, 10);
	VOE_TEST_CHECK_INT(queued[1].from, 2);
	VOE_TEST_CHECK_INT(queued[1].value, 20);
	VOE_TEST_CHECK_INT(queued[2].from, 1);
	VOE_TEST_CHECK_INT(queued[2].value, 30);
	VOE_TEST_CHECK_INT(queued[3].from, 2);
	VOE_TEST_CHECK_INT(queued[3].value, 40);

	// Reading is not draining.
	VOE_TEST_CHECK_INT(voe_ecs_intent_count(world, type), 4);
	voe_ecs_intent_clear(world, type);
	VOE_TEST_CHECK_INT(voe_ecs_intent_count(world, type), 0);

	// And the queue is usable again immediately, from the top.
	from_there(world, type, 50);
	queued = voe_ecs_intent_queue(world, type);
	VOE_TEST_CHECK_INT(voe_ecs_intent_count(world, type), 1);
	VOE_TEST_CHECK_INT(queued[0].value, 50);
}

static void a_full_queue_says_so(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_intent type = voe_ecs_intent_register(world, &nudge_key,
						      sizeof(struct nudge),
						      QUEUED);
	struct nudge nudge = { .from = 1, .value = 0 };

	for (uint32_t i = 0; i < QUEUED; i++) {
		nudge.value = i;
		VOE_TEST_CHECK(voe_ecs_intent_submit(world, type, &nudge));
	}

	VOE_TEST_CHECK(!voe_ecs_intent_submit(world, type, &nudge));

	// A drain is what makes room, which is the honest shape of "the system
	// has not run for long enough".
	voe_ecs_intent_clear(world, type);
	VOE_TEST_CHECK(voe_ecs_intent_submit(world, type, &nudge));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	two_submitters_arrive_in_submission_order(arena);
	a_full_queue_says_so(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
