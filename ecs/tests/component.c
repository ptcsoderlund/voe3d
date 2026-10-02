// Component tables: that a row can be added, read, overwritten and removed, that
// an iteration sees every live row exactly once, that a full table and a stale
// entity are both refused rather than answered, that the world can say what an
// entity is made of without anyone naming a type, and that the two markers a
// registration passes instead of a description answer differently. And that a
// default row, an unsaid row and a needed type come back as they were set, and
// as nothing when they were not.
//
// THE ITERATION CHECK IS THE ONE THAT WOULD CATCH A BROKEN REMOVAL. A removal
// swaps the last row into the hole, so the way to get it wrong is to leave the
// moved row's index pointing at where it used to be — and the symptom of that is
// an iteration that visits something twice or misses it entirely, which is what
// the tally below counts.
//
// THE WALK OVER EVERY TYPE IS THE INSPECTOR'S LOOP, WRITTEN HERE ONCE. It names no
// type: it takes an entity, asks each registered type for a row, and counts what
// answers — which is how a caller that knows nothing about a component finds it.
//
// AND SO IS THE EDIT AT THE BOTTOM OF THIS FILE. A replace intent is three
// numbers — where the row sits in the value, how big the row is, how big the
// value is — and the point of storing them is that a caller can read a row, put
// it back changed, and never name the component's type while doing it. That round
// trip is written out here once, by hand, because there is no system in ecs to
// drain the queue and because getting it wrong in the editor would look like a
// component that does not save rather than like an offset that is four bytes out.
#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <ecs/world.h>

#include <testing/test.h>

#include <stddef.h>
#include <string.h>

#define ENTITIES 16
#define ROWS 4
#define TYPES 3

// Which byte of a row the edit below changes, and to what. A byte by index and
// not a field by name, because that is all a caller walking a description has.
#define CHANGED_BYTE 1
#define CHANGED_TO 0xABu

// Any struct will do; ecs never looks inside one.
struct marker {
	uint32_t tag;
};

static const struct voe_ecs_key marker_key = { "test_marker" };
static const struct voe_ecs_key second_key = { "test_second" };
static const struct voe_ecs_key third_key = { "test_third" };

// The shape ADR-0134 asks for: the entity first and at offset zero, the whole row
// after it. ecs never looks inside one — this file is the only thing here that
// knows the two fields are there.
struct marker_intent {
	voe_ecs_entity entity;
	struct marker row;
};

// Two, because a replace intent that came back as intent zero would look right
// against a world holding only one. The filler is registered first, so the one
// the tests name is not the zeroth.
static const struct voe_ecs_key filler_intent_key = { "test_filler_intent" };
static const struct voe_ecs_key marker_intent_key = { "test_marker_intent" };

// Any record will do either, for the same reason: the world hands the pointer back
// and never follows it, so a record with no fields proves as much as a real one.
static const voe_base_struct_description marker_description = {
	.name = "marker",
	.fields = NULL,
	.field_count = 0,
};

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = ENTITIES,
		.component_types = TYPES,
		.intent_types = 2,
	};

	return voe_ecs_world_new(arena, limits);
}

// The three types every test below that lists them registers, in this order: one
// described, one runtime-only, and one described in a build without descriptions.
static void register_three(voe_ecs_world *world, voe_ecs_type types[TYPES])
{
	types[0] = voe_ecs_component_register(world, &marker_key,
					      sizeof(struct marker), ROWS,
					      &marker_description);
	types[1] = voe_ecs_component_register(world, &second_key,
					      sizeof(struct marker), ROWS,
					      &voe_ecs_runtime_only);
	types[2] = voe_ecs_component_register(world, &third_key,
					      sizeof(struct marker), ROWS,
					      &voe_ecs_description_compiled_out);
}

// The walk: for each registered type, in the world's order, whether the entity
// has a row. Nothing in here knows which types exist.
static uint32_t types_of(const voe_ecs_world *world, voe_ecs_entity entity,
			 bool has[TYPES])
{
	uint32_t found = 0;

	VOE_TEST_CHECK_INT(voe_ecs_component_type_count(world), TYPES);

	for (uint32_t i = 0; i < voe_ecs_component_type_count(world) && i < TYPES;
	     i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		has[i] = voe_ecs_component_get(world, type, entity) != NULL;
		found += has[i];
	}

	return found;
}

static void a_row_goes_in_and_comes_back(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_register(world, &marker_key,
						       sizeof(struct marker),
						       ROWS,
						       &voe_ecs_runtime_only);
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
						       ROWS,
						       &voe_ecs_runtime_only);
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
						       ROWS,
						       &voe_ecs_runtime_only);
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

// Three in, three out, in the order they went in, each with its own key behind it.
static void the_world_lists_its_types_in_registration_order(
	voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type types[TYPES];
	const struct voe_ecs_key *keys[TYPES] = { &marker_key, &second_key,
						  &third_key };

	VOE_TEST_CHECK_INT(voe_ecs_component_type_count(world), 0);
	register_three(world, types);
	VOE_TEST_CHECK_INT(voe_ecs_component_type_count(world), TYPES);

	for (uint32_t i = 0; i < TYPES; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		VOE_TEST_CHECK_INT(type.value, types[i].value);
		VOE_TEST_CHECK(voe_ecs_component_key(world, type) == keys[i]);
	}
}

// Two of the three on one entity and the missing one on another, so a walk that
// answered for the world rather than for the entity would find three.
static void an_entity_is_found_to_have_exactly_what_it_was_given(
	voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type types[TYPES];
	voe_ecs_entity thing = { 0 };
	voe_ecs_entity other = { 0 };
	struct marker value = { .tag = 1 };
	bool has[TYPES] = { false };

	register_three(world, types);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &other));
	VOE_TEST_CHECK(voe_ecs_component_add(world, types[0], thing, &value));
	VOE_TEST_CHECK(voe_ecs_component_add(world, types[2], thing, &value));
	VOE_TEST_CHECK(voe_ecs_component_add(world, types[1], other, &value));

	VOE_TEST_CHECK_INT(types_of(world, thing, has), 2);
	VOE_TEST_CHECK(has[0]);
	VOE_TEST_CHECK(!has[1]);
	VOE_TEST_CHECK(has[2]);
}

static void a_description_comes_back_as_it_was_registered(
	voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type types[TYPES];

	register_three(world, types);
	VOE_TEST_CHECK(voe_ecs_component_description(world, types[0]) ==
		       &marker_description);
	VOE_TEST_CHECK(voe_ecs_component_description(world, types[1]) == NULL);
	VOE_TEST_CHECK(voe_ecs_component_description(world, types[2]) == NULL);
}

// The two NULLs above are not the same answer, and this is where they part: only
// the type that said runtime-only is runtime-only. The compiled-out one is still
// authored data, and a writer asking this is how it tells that apart from state
// nobody saves.
static void only_a_runtime_only_type_says_it_is_one(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type types[TYPES];

	register_three(world, types);
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, types[0]));
	VOE_TEST_CHECK(voe_ecs_component_runtime_only(world, types[1]));
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, types[2]));
}

// The destroyed entity's slot is taken again by something that has all three, so
// every row the old id could find is the newcomer's — and it must find none.
static void a_stale_entity_is_made_of_nothing(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type types[TYPES];
	voe_ecs_entity stale = { 0 };
	voe_ecs_entity newcomer = { 0 };
	struct marker value = { .tag = 1 };
	bool has[TYPES] = { false };

	register_three(world, types);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &stale));
	VOE_TEST_CHECK(voe_ecs_component_add(world, types[0], stale, &value));
	voe_ecs_entity_destroy(world, stale);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &newcomer));
	VOE_TEST_CHECK_INT(newcomer.index, stale.index);
	for (uint32_t i = 0; i < TYPES; i++)
		VOE_TEST_CHECK(voe_ecs_component_add(world, types[i], newcomer,
						     &value));

	VOE_TEST_CHECK_INT(types_of(world, newcomer, has), TYPES);
	VOE_TEST_CHECK_INT(types_of(world, stale, has), 0);
}

// The two registrations every replace test makes: the component, then the intent
// that replaces a row of it, with a filler intent in front so the one under test
// is not intent zero.
static voe_ecs_intent register_marker_and_its_intent(voe_ecs_world *world,
						     voe_ecs_type *type)
{
	*type = voe_ecs_component_register(world, &marker_key,
					   sizeof(struct marker), ROWS,
					   &marker_description);
	(void)voe_ecs_intent_register(world, &filler_intent_key,
				      sizeof(struct marker), ROWS);
	return voe_ecs_intent_register(world, &marker_intent_key,
				       sizeof(struct marker_intent), ROWS);
}

static void a_type_hands_back_the_intent_it_was_given(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type;
	voe_ecs_intent intent = register_marker_and_its_intent(world, &type);
	voe_ecs_replace replace;

	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(struct marker_intent, row));

	replace = voe_ecs_component_replace(world, type);
	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT(replace.intent.value, intent.value);
	VOE_TEST_CHECK_INT(replace.row_offset,
			   offsetof(struct marker_intent, row));
	VOE_TEST_CHECK_INT(replace.row_size, sizeof(struct marker));
	VOE_TEST_CHECK_INT(replace.value_size, sizeof(struct marker_intent));

	// The sizes are what a caller sizes a buffer with, so the row has to fit
	// inside the value at the offset — the same comparison _replace_set made.
	VOE_TEST_CHECK(replace.row_offset + replace.row_size <=
		       replace.value_size);
	VOE_TEST_CHECK(replace.row_offset >= sizeof(voe_ecs_entity));
}

// A component nothing edits is shown and not edited, and that is four zeroes and
// a false rather than anything a caller has to remember not to use.
static void a_type_without_one_says_so_and_nothing_else(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type types[TYPES];
	voe_ecs_replace replace;

	register_three(world, types);

	for (uint32_t i = 0; i < TYPES; i++) {
		replace = voe_ecs_component_replace(world, types[i]);
		VOE_TEST_CHECK(!replace.set);
		VOE_TEST_CHECK_INT(replace.intent.value, 0);
		VOE_TEST_CHECK_INT(replace.row_offset, 0);
		VOE_TEST_CHECK_INT(replace.row_size, 0);
		VOE_TEST_CHECK_INT(replace.value_size, 0);
	}
}

// THE ROUND TRIP THE EDITOR WILL MAKE, ONCE AND BY HAND. Everything between the
// two _get calls is written as a tool would write it: the type's own name appears
// only where the world is set up, and from _replace onwards the row is bytes at
// an offset. The drain at the end is the owning system's work in any real folder
// and is done here because ecs has no systems in it.
static void a_row_is_edited_through_its_replace_intent(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type;
	voe_ecs_intent intent = register_marker_and_its_intent(world, &type);
	voe_ecs_entity thing = { 0 };
	// The row the entity starts with, spelled as bytes and handed to _add as
	// bytes. What this checks at the end is which bytes moved, so the bytes
	// are what it states up front — and ecs copies the registered size
	// without looking inside, which is the same reason it can.
	static const unsigned char start[sizeof(struct marker)] = { 0x01, 0x02,
								    0x03, 0x04 };
	unsigned char value[sizeof(struct marker_intent)] = { 0 };
	const unsigned char *queued;
	const unsigned char *now;
	voe_ecs_replace replace;
	const void *read;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_ecs_component_add(world, type, thing, start));
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(struct marker_intent, row));

	replace = voe_ecs_component_replace(world, type);
	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT(replace.value_size, sizeof value);
	if (!replace.set)
		return;

	// Read the row, copy it into a zeroed value at the offset the type gave,
	// and put the entity at offset zero where it always is.
	read = voe_ecs_component_get(world, type, thing);
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	memcpy(value, &thing, sizeof thing);
	memcpy(value + replace.row_offset, read, replace.row_size);

	// One byte of the copy, and nothing else in it.
	value[replace.row_offset + CHANGED_BYTE] = CHANGED_TO;
	VOE_TEST_CHECK(voe_ecs_intent_submit(world, intent, value));
	VOE_TEST_CHECK_INT(voe_ecs_intent_count(world, intent), 1);

	// The drain: the entity out of the front of each queued value, the row out
	// of the offset behind it, all of it, then empty.
	queued = voe_ecs_intent_queue(world, intent);
	for (uint32_t i = 0; i < voe_ecs_intent_count(world, intent); i++) {
		const unsigned char *one =
			queued + (size_t)i * replace.value_size;
		voe_ecs_entity named;

		memcpy(&named, one, sizeof named);
		VOE_TEST_CHECK(voe_ecs_component_set(world, type, named,
						     one + replace.row_offset));
	}
	voe_ecs_intent_clear(world, intent);

	// That byte and no other. Byte by byte rather than field by field, because
	// "no other" is the half of the claim a field comparison would not make.
	read = voe_ecs_component_get(world, type, thing);
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	now = read;
	VOE_TEST_CHECK_INT(now[CHANGED_BYTE], CHANGED_TO);
	for (size_t i = 0; i < sizeof(struct marker); i++) {
		if (i == CHANGED_BYTE)
			continue;
		VOE_TEST_CHECK_INT(now[i], start[i]);
	}
}

// The default comes back byte for byte, from the world's own copy: the row it was
// set from is overwritten before it is read.
static void a_default_comes_back_as_it_was_set(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type types[TYPES];
	unsigned char row[sizeof(struct marker)] = { 0x0A, 0x0B, 0x0C, 0x0D };
	static const unsigned char expected[sizeof(struct marker)] = {
		0x0A, 0x0B, 0x0C, 0x0D
	};
	const unsigned char *read;

	register_three(world, types);
	voe_ecs_component_default_set(world, types[0], row);
	memset(row, 0, sizeof row);

	read = voe_ecs_component_default(world, types[0]);
	VOE_TEST_CHECK(read != NULL);
	if (read != NULL)
		for (size_t i = 0; i < sizeof expected; i++)
			VOE_TEST_CHECK_INT(read[i], expected[i]);
	VOE_TEST_CHECK(voe_ecs_component_default(world, types[1]) == NULL);
}

// The unsaid row is unset as NULL, comes back byte for byte from memory of its
// own, and setting it leaves the default row as it was.
static void an_unsaid_row_comes_back_as_it_was_set(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type types[TYPES];
	static const unsigned char fallback[sizeof(struct marker)] = {
		0x01, 0x02, 0x03, 0x04
	};
	unsigned char row[sizeof(struct marker)] = { 0x05, 0x06, 0x07, 0x08 };
	static const unsigned char expected[sizeof(struct marker)] = {
		0x05, 0x06, 0x07, 0x08
	};
	const unsigned char *read;
	const unsigned char *fallback_read;

	register_three(world, types);
	VOE_TEST_CHECK(voe_ecs_component_unsaid(world, types[0]) == NULL);

	voe_ecs_component_default_set(world, types[0], fallback);
	voe_ecs_component_unsaid_set(world, types[0], row);
	memset(row, 0, sizeof row);

	read = voe_ecs_component_unsaid(world, types[0]);
	fallback_read = voe_ecs_component_default(world, types[0]);
	VOE_TEST_CHECK(read != NULL);
	VOE_TEST_CHECK(read != fallback_read);
	if (read != NULL)
		for (size_t i = 0; i < sizeof expected; i++)
			VOE_TEST_CHECK_INT(read[i], expected[i]);
	VOE_TEST_CHECK(fallback_read != NULL);
	if (fallback_read != NULL)
		for (size_t i = 0; i < sizeof fallback; i++)
			VOE_TEST_CHECK_INT(fallback_read[i], fallback[i]);
	VOE_TEST_CHECK(voe_ecs_component_unsaid(world, types[1]) == NULL);
}

// Type zero is the one needed, so a "needs" that came back as a zeroed type
// without being set would look right — which the unset types' false rules out.
static void a_need_comes_back_as_it_was_set(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type types[TYPES];
	voe_ecs_type needed = { .value = UINT32_MAX };

	register_three(world, types);
	voe_ecs_component_needs_set(world, types[2], types[0]);

	VOE_TEST_CHECK(voe_ecs_component_needs(world, types[2], &needed));
	VOE_TEST_CHECK_INT(needed.value, types[0].value);
	VOE_TEST_CHECK(!voe_ecs_component_needs(world, types[0], &needed));
	VOE_TEST_CHECK(!voe_ecs_component_needs(world, types[1], &needed));
}

// The pointer, not a copy: the string is the declaring folder's.
static void menu_path(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type types[TYPES];
	static const char path[] = "Rendering / Shape";

	register_three(world, types);
	VOE_TEST_CHECK(voe_ecs_component_menu(world, types[0]) == NULL);

	voe_ecs_component_menu_set(world, types[0], path);

	VOE_TEST_CHECK(voe_ecs_component_menu(world, types[0]) == path);
	VOE_TEST_CHECK(voe_ecs_component_menu(world, types[1]) == NULL);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	a_row_goes_in_and_comes_back(arena);
	an_iteration_sees_every_live_row_once(arena);
	a_full_table_and_a_stale_entity_are_both_refused(arena);
	the_world_lists_its_types_in_registration_order(arena);
	an_entity_is_found_to_have_exactly_what_it_was_given(arena);
	a_description_comes_back_as_it_was_registered(arena);
	only_a_runtime_only_type_says_it_is_one(arena);
	a_stale_entity_is_made_of_nothing(arena);
	a_type_hands_back_the_intent_it_was_given(arena);
	a_type_without_one_says_so_and_nothing_else(arena);
	a_row_is_edited_through_its_replace_intent(arena);
	a_default_comes_back_as_it_was_set(arena);
	an_unsaid_row_comes_back_as_it_was_set(arena);
	a_need_comes_back_as_it_was_set(arena);
	menu_path(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
