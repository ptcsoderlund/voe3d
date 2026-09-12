// The identity system: registration, the one direct creation call, the drain
// that settles what it applies, and the report that settling wrote.
//
// THE DRAIN IS THE WHOLE SYSTEM, as it is for transform. There is nothing per
// frame to compute: an identity is two values that sit where they were put.
//
// AN INTENT FOR AN ENTITY WITH NO IDENTITY IS DROPPED, and the drain asks for the
// current row before it settles rather than letting voe_ecs_component_set refuse
// it afterwards — it needs that row anyway, because the id it puts back is the
// one already there. A destroyed entity and one that was never authored both
// come back NULL, and both are ordinary rather than wrong — see the header.
//
// THE ID IS PUT BACK AND THE NAME IS KEPT, WHICH IS WHY THIS IS NOT A REFUSAL.
// An intent carries the whole row, so a submitter that only meant to rename still
// hands over an id; one that hands over the wrong id has made a mistake in one
// field and an edit in the other, and dropping the intent would throw away the
// edit. scene/identity_component.h holds that reasoning at length.
//
// THE RUN FLAG AND THE COUNT ARE FILE-SCOPE STATICS AND THEREFORE PER PROCESS.
// Two worlds in one process share them. The header says why that is enough.
//
// THE IDENTITY REGISTERS ITS DESCRIPTION WHEN THE BUILD HAS ONE, AND NULL WHEN IT
// DOES NOT. The accessor only exists when descriptions are compiled in
// (base/describe.h), so the #if below is the condition that header tests. The
// table it returns is this file's own static copy and lives as long as the program.
#include <base/assert.h>
#include <ecs/intent.h>
#include <scene/identity_system.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// One intent per identity per frame is the sizing this assumes, so the queue is
// as long as the table.
static const struct voe_ecs_key identity_intent_key = {
	"voe_scene_identity_intent"
};

// Long enough for either correction's sentence with a 20-digit id in it.
#define REASON 96

// Whether the last drain corrected anything, and how many intents the run it
// belongs to has corrected so far. Per process — see the header.
static bool in_run;
static uint32_t run_count;

static bool terminated(const char *name)
{
	return memchr(name, '\0', VOE_SCENE_IDENTITY_NAME) != NULL;
}

static const voe_base_struct_description *identity_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_identity_description();
#else
	return NULL;
#endif
}

// What the report calls this entity: the name the intent itself carries, when
// that name is readable, and the entity's index and generation when it is not.
// Read from the intent as submitted, so it must be taken before the name is cut.
static void label_of(voe_ecs_entity entity, const voe_scene_identity *submitted,
		     char *out, size_t size)
{
	if (terminated(submitted->name))
		(void)snprintf(out, size, "%s", submitted->name);
	else
		(void)snprintf(out, size, "%uv%u", entity.index,
			       entity.generation);
}

// Corrects the row in place towards the one nearest valid value each rule has,
// in the order scene/identity_component.h lists them. True when it corrected
// something, and then `reason` holds the sentence for the first correction — the
// report has one line to spend and the first fault is the one that describes the
// intent best.
static bool settled(voe_scene_identity *identity,
		    const voe_scene_identity *current, char *reason, size_t size)
{
	bool corrected = false;

	if (!terminated(identity->name)) {
		identity->name[VOE_SCENE_IDENTITY_NAME - 1] = '\0';
		(void)snprintf(reason, size,
			       "name was not terminated, cut to %d bytes",
			       VOE_SCENE_IDENTITY_NAME - 1);
		corrected = true;
	}

	if (identity->id != current->id) {
		if (!corrected)
			(void)snprintf(reason, size,
				       "id cannot be replaced, kept %" PRIu64,
				       current->id);
		identity->id = current->id;
		corrected = true;
	}

	return corrected;
}

void voe_scene_identity_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering identities in no world");

	type = voe_ecs_component_register(world, &voe_scene_identity_key,
					  sizeof(voe_scene_identity), capacity,
					  identity_description());
	intent = voe_ecs_intent_register(world, &identity_intent_key,
					 sizeof(voe_scene_identity_intent),
					 capacity);
	voe_ecs_component_replace_set(
		world, type, intent,
		offsetof(voe_scene_identity_intent, identity));
}

bool voe_scene_identity_add(voe_ecs_world *world, voe_ecs_entity entity,
			    voe_scene_identity identity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding an identity to no world");
	VOE_BASE_ASSERT(terminated(identity.name),
			"a name with no terminating zero in its 64 bytes — see scene/identity_component.h");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_identity_key),
		entity, &identity);
}

bool voe_scene_identity_submit(voe_ecs_world *world,
			       voe_scene_identity_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting an identity to no world");
	VOE_BASE_ASSERT(terminated(intent.identity.name),
			"a name with no terminating zero in its 64 bytes — see scene/identity_component.h");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &identity_intent_key),
		&intent);
}

void voe_scene_identity_system_run(voe_ecs_world *world)
{
	voe_ecs_intent queue;
	voe_ecs_type type;
	const voe_scene_identity_intent *intents;
	uint32_t count;
	uint32_t corrected = 0;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the identity system on no world");

	queue = voe_ecs_intent_type(world, &identity_intent_key);
	type = voe_ecs_component_type(world, &voe_scene_identity_key);
	intents = voe_ecs_intent_queue(world, queue);
	count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_identity *current =
			voe_ecs_component_get(world, type, intents[i].entity);
		voe_scene_identity row = intents[i].identity;
		char reason[REASON];

		if (current == NULL)
			continue;

		if (settled(&row, current, reason, sizeof reason)) {
			// One line at the start of a run and no more: the first
			// correction says what went wrong, and the count at the
			// end says how much of it there was.
			if (!in_run && corrected == 0) {
				char label[VOE_SCENE_IDENTITY_NAME];

				label_of(intents[i].entity,
					 &intents[i].identity, label,
					 sizeof label);
				fprintf(stderr,
					"warning: voe_scene_identity: entity %s: %s\n",
					label, reason);
			}
			corrected++;
		}

		(void)voe_ecs_component_set(world, type, intents[i].entity,
					    &row);
	}

	if (corrected > 0) {
		in_run = true;
		run_count += corrected;
	} else if (in_run) {
		fprintf(stderr,
			"warning: voe_scene_identity: %" PRIu32
			" intents settled in that run\n",
			run_count);
		in_run = false;
		run_count = 0;
	}

	voe_ecs_intent_clear(world, queue);
}
