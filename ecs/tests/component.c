// Component tables: that a row can be added, read, overwritten and removed, that
// an iteration sees every live row exactly once, and that a full table and a
// stale entity are both refused rather than answered.
//
// THE ITERATION CHECK IS THE ONE THAT WOULD CATCH A BROKEN REMOVAL. A removal
// swaps the last row into the hole, so the way to get it wrong is to leave the
// moved row's index pointing at where it used to be — and the symptom of that is
// an iteration that visits something twice or misses it entirely, which is what
// the tally below counts.
#include <base/arena.h>
#include <ecs/component.h>
#include <ecs/world.h>

#include <testing/test.h>

#define ENTITIES 16
#define ROWS 4

// Any struct will do; ecs never looks inside one.
struct marker {
	uint32_t tag;
};

static const struct voe_ecs_key marker_key = { "test_marker" };

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = ENTITIES,
		.component_types = 2,
		.intent_types = 1,
	};

	return voe_ecs_world_new(arena, limits);
}

static void a_row_goes_in_and_comes_back(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(world, &marker_key,
						       sizeof(struct marker),
						       ROWS);
	voe_ecs_entity thing = { 0 };
	struct marker value = { .tag = 7 };
	const struct marker *read;

	// The type comes back by key, from anywhere, without being carried
	// around: that is what lets a module's own API take only the world.
	VOE_TEST_CHECK_INT(voe_ecs_component_type(world, &marker_key).value,
			   type.value);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_ecs_component_get(world, type, thing) == NULL);

	VOE_TEST_CHECK(voe_ecs_component_add(world, type, thing, &value));
	read = voe_ecs_component_get(world, type, thing);
	VOE_TEST_CHECK(read != NULL);
	if (read != NULL)
		VOE_TEST_CHECK_INT(read->tag, 7);
	VOE_TEST_CHECK_INT(voe_ecs_component_count(world, type), 1);

	value.tag = 9;
	VOE_TEST_CHECK(voe_ecs_component_set(world, type, thing, &value));
	read = voe_ecs_component_get(world, type, thing);
	if (read != NULL)
		VOE_TEST_CHECK_INT(read->tag, 9);

	VOE_TEST_CHECK(voe_ecs_component_remove(world, type, thing));
	VOE_TEST_CHECK(voe_ecs_component_get(world, type, thing) == NULL);
	VOE_TEST_CHECK_INT(voe_ecs_component_count(world, type), 0);

	// Nothing to remove, nothing to set.
	VOE_TEST_CHECK(!voe_ecs_component_remove(world, type, thing));
	VOE_TEST_CHECK(!voe_ecs_component_set(world, type, thing, &value));
}

// Every live row once: the tag of each row is the entity's own index, so a walk
// can tally what it saw and the tally can be compared against what is alive.
static void an_iteration_sees_every_live_row_once(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(world, &marker_key,
						       sizeof(struct marker),
						       ROWS);
	voe_ecs_entity held[ROWS];
	uint32_t seen[ROWS] = { 0 };
	const struct marker *rows;
	const voe_ecs_entity *owners;

	for (uint32_t i = 0; i < ROWS; i++) {
		struct marker value = { .tag = i };

		VOE_TEST_CHECK(voe_ecs_entity_create(world, &held[i]));
		VOE_TEST_CHECK(voe_ecs_component_add(world, type, held[i],
						     &value));
	}

	// Remove the one in the middle, which is the case where the last row has
	// to move and its index has to follow it.
	VOE_TEST_CHECK(voe_ecs_component_remove(world, type, held[1]));
	VOE_TEST_CHECK_INT(voe_ecs_component_count(world, type), ROWS - 1);

	rows = voe_ecs_component_rows(world, type);
	owners = voe_ecs_component_entities(world, type);
	for (uint32_t row = 0; row < voe_ecs_component_count(world, type);
	     row++) {
		VOE_TEST_CHECK(rows[row].tag < ROWS);
		if (rows[row].tag >= ROWS)
			continue;

		seen[rows[row].tag]++;

		// The row and the entity beside it are the same object, and a
		// system that walks one table and looks the second component up
		// leans on exactly this.
		VOE_TEST_CHECK_INT(owners[row].index, held[rows[row].tag].index);
		VOE_TEST_CHECK(voe_ecs_component_get(world, type, owners[row]) ==
			       &rows[row]);
	}

	VOE_TEST_CHECK_INT(seen[0], 1);
	VOE_TEST_CHECK_INT(seen[1], 0);
	VOE_TEST_CHECK_INT(seen[2], 1);
	VOE_TEST_CHECK_INT(seen[3], 1);
}

static void a_full_table_and_a_stale_entity_are_both_refused(
	voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(world, &marker_key,
						       sizeof(struct marker),
						       ROWS);
	struct marker value = { .tag = 1 };
	voe_ecs_entity held[ROWS];
	voe_ecs_entity extra = { 0 };

	for (uint32_t i = 0; i < ROWS; i++) {
		VOE_TEST_CHECK(voe_ecs_entity_create(world, &held[i]));
		VOE_TEST_CHECK(voe_ecs_component_add(world, type, held[i],
						     &value));
	}

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &extra));
	VOE_TEST_CHECK(!voe_ecs_component_add(world, type, extra, &value));

	// Destroying an entity takes its row with it, which is both what frees
	// the room below and what stops a dead entity's row from being walked.
	voe_ecs_entity_destroy(world, held[0]);
	VOE_TEST_CHECK_INT(voe_ecs_component_count(world, type), ROWS - 1);
	VOE_TEST_CHECK(voe_ecs_component_add(world, type, extra, &value));

	// And the id that used to name that row names nothing now.
	VOE_TEST_CHECK(voe_ecs_component_get(world, type, held[0]) == NULL);
	VOE_TEST_CHECK(!voe_ecs_component_add(world, type, held[0], &value));
	VOE_TEST_CHECK(!voe_ecs_component_set(world, type, held[0], &value));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	a_row_goes_in_and_comes_back(arena);
	an_iteration_sees_every_live_row_once(arena);
	a_full_table_and_a_stale_entity_are_both_refused(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
