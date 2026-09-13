// The identity: that a name and an id round-trip, that the only way to change
// one is an intent the system drains, and that the drain corrects rather than
// refuses — a name that was not terminated, and an id that may not be replaced.
//
// THE RAW SUBMIT IS THE POINT OF HALF THIS FILE. The typed calls assert a
// terminated name, so a test that only used them could never reach the drain's
// correction. What reaches the drain in a running program is whatever a tool
// that knows only the offsets put in the queue (ecs/component.h), so the checks
// below submit through voe_ecs_intent_submit with the bytes a tool would write.
//
// THE DESCRIPTION IS SWITCHED ON HERE, WHATEVER THE BUILD SAID, for the reason
// scene/tests/transform.c gives at length: check.cmake builds without
// descriptions, and a check that followed the build would never run on the one
// run that gates a card. WHAT THE BUILD SAID IS KEPT FIRST, because the identity
// scene/src registers follows the build and not this file.
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
#define BUILD_DESCRIBES true
#else
#define BUILD_DESCRIBES false
#endif
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <ecs/world.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>

#include <testing/test.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define IDENTITIES 8

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = IDENTITIES,
		.component_types = 2,
		.intent_types = 2,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_identity_register(world, IDENTITIES);
	return world;
}

static voe_scene_identity named(uint64_t id, const char *name)
{
	voe_scene_identity identity = { .id = id };

	VOE_TEST_CHECK(strlen(name) < VOE_SCENE_IDENTITY_NAME);
	memcpy(identity.name, name, strlen(name));
	return identity;
}

// An entity with an identity, ready to be edited.
static voe_ecs_entity authored(voe_ecs_world *world, uint64_t id,
			       const char *name)
{
	voe_ecs_entity thing = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_scene_identity_add(world, thing, named(id, name)));
	return thing;
}

static void check_name(const char *actual, const char *expected)
{
	VOE_TEST_CHECK(strcmp(actual, expected) == 0);
	if (strcmp(actual, expected) != 0)
		fprintf(stderr, "      actual:   \"%s\"\n      expected: \"%s\"\n",
			actual, expected);
}

// The bytes a tool that knows only the offsets would write, straight into the
// queue. This is the one door the drain's corrections exist for.
static bool submit_raw(voe_ecs_world *world, voe_ecs_entity entity,
		       voe_scene_identity identity)
{
	voe_scene_identity_intent intent = { .entity = entity,
					     .identity = identity };
	voe_ecs_type type = voe_ecs_component_type(world,
						   &voe_scene_identity_key);
	voe_ecs_intent queue = voe_ecs_component_replace(world, type).intent;

	return voe_ecs_intent_submit(world, queue, &intent);
}

static void an_identity_round_trips_through_the_table(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity thing;
	voe_ecs_entity plain = { 0 };
	const voe_scene_identity *read;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &plain));
	VOE_TEST_CHECK(voe_scene_identity_get(world, plain) == NULL);

	thing = authored(world, 7, "Cube");

	read = voe_scene_identity_get(world, thing);
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;

	VOE_TEST_CHECK_INT((long long)read->id, 7);
	check_name(read->name, "Cube");

	// The entity with no identity is not in the table, which is the whole of
	// what "its presence means authored" costs to check.
	VOE_TEST_CHECK_INT(voe_scene_identity_count(world), 1);
	VOE_TEST_CHECK(voe_scene_identity_rows(world) == read);
	VOE_TEST_CHECK_INT(voe_scene_identity_entities(world)[0].index,
			   thing.index);
}

static void an_intent_lands_only_when_the_system_runs(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity thing = authored(world, 7, "Cube");
	const voe_scene_identity *read;

	VOE_TEST_CHECK(voe_scene_identity_submit(
		world, (voe_scene_identity_intent){
			       .entity = thing,
			       .identity = named(7, "Renamed") }));

	read = voe_scene_identity_get(world, thing);
	if (read != NULL)
		check_name(read->name, "Cube");

	voe_scene_identity_system_run(world);

	read = voe_scene_identity_get(world, thing);
	if (read != NULL) {
		check_name(read->name, "Renamed");
		VOE_TEST_CHECK_INT((long long)read->id, 7);
	}
}

// An entity that never had an identity is not given one by an intent: creation
// is the direct call and nothing else.
static void an_intent_for_an_unauthored_entity_is_dropped(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity plain = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &plain));
	VOE_TEST_CHECK(voe_scene_identity_submit(
		world, (voe_scene_identity_intent){
			       .entity = plain,
			       .identity = named(1, "Ghost") }));
	voe_scene_identity_system_run(world);

	VOE_TEST_CHECK(voe_scene_identity_get(world, plain) == NULL);
	VOE_TEST_CHECK_INT(voe_scene_identity_count(world), 0);
}

// The door an inspector knocks on: the world says which intent replaces a whole
// identity row, and where in that intent the row sits.
static void the_world_names_the_intent_that_replaces_an_identity(
	voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_type(world,
						   &voe_scene_identity_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT(
		(long long)replace.row_offset,
		(long long)offsetof(voe_scene_identity_intent, identity));
	VOE_TEST_CHECK_INT((long long)replace.row_size,
			   (long long)sizeof(voe_scene_identity));
	VOE_TEST_CHECK_INT((long long)replace.value_size,
			   (long long)sizeof(voe_scene_identity_intent));
}

// A name with no terminating zero arrives terminated, and the id beside it is
// untouched: the drain corrects the field that is wrong and keeps the rest of
// the row as submitted.
static void an_unterminated_name_arrives_cut(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity thing = authored(world, 7, "Cube");
	voe_scene_identity raw = { .id = 7 };
	const voe_scene_identity *read;

	memset(raw.name, 'x', VOE_SCENE_IDENTITY_NAME);
	VOE_TEST_CHECK(submit_raw(world, thing, raw));
	voe_scene_identity_system_run(world);

	read = voe_scene_identity_get(world, thing);
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;

	VOE_TEST_CHECK_INT((long long)strlen(read->name),
			   VOE_SCENE_IDENTITY_NAME - 1);
	VOE_TEST_CHECK_INT(read->name[VOE_SCENE_IDENTITY_NAME - 1], 0);
	VOE_TEST_CHECK_INT(read->name[0], 'x');
	VOE_TEST_CHECK_INT((long long)read->id, 7);
}

// The id is read-only: a replace carrying another one is put back, and the name
// it came with still lands, because the submitter meant that part.
static void a_replaced_id_is_put_back(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity thing = authored(world, 2, "Cube_2");
	const voe_scene_identity *read;

	VOE_TEST_CHECK(voe_scene_identity_submit(
		world, (voe_scene_identity_intent){
			       .entity = thing,
			       .identity = named(99, "Cube_2_renamed") }));
	voe_scene_identity_system_run(world);

	read = voe_scene_identity_get(world, thing);
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;

	VOE_TEST_CHECK_INT((long long)read->id, 2);
	check_name(read->name, "Cube_2_renamed");
}

static void check_field(const voe_base_field_description *actual,
			const char *name, voe_base_field_kind kind,
			size_t offset, uint32_t count, bool read_only)
{
	VOE_TEST_CHECK(strcmp(actual->name, name) == 0);
	if (strcmp(actual->name, name) != 0)
		fprintf(stderr, "      actual:   \"%s\"\n      expected: \"%s\"\n",
			actual->name, name);
	VOE_TEST_CHECK_INT(actual->kind, kind);
	VOE_TEST_CHECK_INT((long long)actual->offset, (long long)offset);
	VOE_TEST_CHECK_INT(actual->count, count);
	VOE_TEST_CHECK_INT(actual->read_only, read_only);
}

// Two fields, in the order they are declared: the id read-only, because nothing
// but creation may set it, and the name sixty-four characters that may be edited.
static void check_description(const voe_base_struct_description *description)
{
	const voe_base_field_description *fields = description->fields;

	VOE_TEST_CHECK(strcmp(description->name, "voe_scene_identity") == 0);
	VOE_TEST_CHECK_INT(description->field_count, 2);
	if (description->field_count != 2)
		return;

	check_field(&fields[0], "id", VOE_BASE_FIELD_UINT64,
		    offsetof(voe_scene_identity, id), 1, true);
	check_field(&fields[1], "name", VOE_BASE_FIELD_CHAR,
		    offsetof(voe_scene_identity, name),
		    VOE_SCENE_IDENTITY_NAME, false);
}

static void the_description_is_the_struct_the_compiler_laid_out(void)
{
	check_description(voe_scene_identity_description());
}

// The inspector, minus the drawing: ask the world what an entity is made of
// without naming a type, and reach the field list from the answer.
static void the_world_hands_back_the_identitys_field_list(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity thing = authored(world, 7, "Cube");
	const voe_base_struct_description *found = NULL;
	bool runtime_only = true;
	uint32_t had = 0;

	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_get(world, type, thing) == NULL)
			continue;

		had++;
		found = voe_ecs_component_description(world, type);
		runtime_only = voe_ecs_component_runtime_only(world, type);
		VOE_TEST_CHECK(voe_ecs_component_key(world, type) ==
			       &voe_scene_identity_key);
	}

	VOE_TEST_CHECK_INT(had, 1);

	// Not runtime-only in either build: with descriptions off the type is
	// still authored data, and a NULL here that also said runtime-only would
	// be a scene that saves nothing and says nothing.
	VOE_TEST_CHECK(!runtime_only);

	if (!BUILD_DESCRIBES) {
		VOE_TEST_CHECK(found == NULL);
		return;
	}

	VOE_TEST_CHECK(found != NULL);
	if (found != NULL)
		check_description(found);
}

// A drain with nothing in it, which is what closes a run of corrections and
// writes its count. Between the two corrections below so that each of them
// starts a run of its own, and every line the system can write is written once
// in a passing run — the format is for a person reading stderr, and a format
// nothing ever prints is a format nobody has read.
static void a_quiet_drain(voe_base_arena *arena)
{
	voe_scene_identity_system_run(world_of(arena));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	the_description_is_the_struct_the_compiler_laid_out();
	the_world_hands_back_the_identitys_field_list(arena);
	the_world_names_the_intent_that_replaces_an_identity(arena);
	an_identity_round_trips_through_the_table(arena);
	an_intent_lands_only_when_the_system_runs(arena);
	an_intent_for_an_unauthored_entity_is_dropped(arena);

	an_unterminated_name_arrives_cut(arena);
	a_quiet_drain(arena);
	a_replaced_id_is_put_back(arena);
	a_quiet_drain(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
