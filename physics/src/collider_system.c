// The collider system: registration, the one direct creation call, the drain
// that settles what it applies, and the report that settling writes.
//
// THE DRAIN IS THE WHOLE SYSTEM. It reads the queue in order, settles each
// intent, writes it or keeps the row, and clears. A shape in the world is worked
// out on demand (physics/shape.h), so nothing is computed per step.
//
// THE RUN FLAG AND THE COUNTERS ARE FILE-SCOPE STATICS AND THEREFORE PER
// PROCESS, as transform's are; scene/transform_system.h says why that is enough.
#include <base/assert.h>
#include <base/report.h>
#include <ecs/intent.h>
#include <physics/collider_system.h>
#include <scene/transform_component.h>

#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

static const struct voe_ecs_key collider_intent_key = {
	"voe_physics_collider_intent"
};

// Long enough for the longest sentence below with three `%g` floats in it.
#define REASON 160

// What settling decided about one intent: nothing to say, applied with a
// warning, or refused and the row kept.
typedef enum {
	SETTLED_NOTHING,
	SETTLED_WARNED,
	SETTLED_KEPT,
} settling;

static bool in_run;
static uint32_t run_count;
static bool run_kept;

static const voe_base_struct_description *collider_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_physics_collider_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_physics_collider_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_type transform;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering colliders in no world");

	// Asserts when no transform was registered: a collider needs one.
	transform = voe_ecs_component_type(world, &voe_scene_transform_key);
	type = voe_ecs_component_register(world, &voe_physics_collider_key,
					  sizeof(voe_physics_collider),
					  capacity, collider_description());
	intent = voe_ecs_intent_register(world, &collider_intent_key,
					 sizeof(voe_physics_collider_intent),
					 capacity);
	voe_ecs_component_replace_set(
		world, type, intent,
		offsetof(voe_physics_collider_intent, collider));
	voe_ecs_component_default_set(
		world, type,
		&(voe_physics_collider){ .kind = VOE_PHYSICS_COLLIDER_BOX,
					 .size = { 1.0f, 1.0f, 1.0f },
					 .trigger = false });
	voe_ecs_component_needs_set(world, type, transform);
	voe_ecs_component_menu_set(world, type, "Physics / Collider");
}

bool voe_physics_collider_add(voe_ecs_world *world, voe_ecs_entity entity,
			      voe_physics_collider collider)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a collider to no world");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_physics_collider_key),
		entity, &collider);
}

bool voe_physics_collider_submit(voe_ecs_world *world,
				 voe_physics_collider_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a collider to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &collider_intent_key),
		&intent);
}

// Whether a size component is one a shape can be built from. Written so a NaN
// fails it.
static bool usable(float v)
{
	return isfinite(v) && v >= 0.0f;
}

// Decides about one intent and writes the sentence the report prints for it.
static settling settled(voe_physics_collider collider, char *reason,
			size_t size)
{
	voe_math_float3 s = collider.size;

	if (!(usable(s.x) && usable(s.y) && usable(s.z))) {
		(void)snprintf(reason, size,
			       "size (%g, %g, %g) is not finite or below 0, row kept",
			       (double)s.x, (double)s.y, (double)s.z);
		return SETTLED_KEPT;
	}
	if (collider.kind < VOE_PHYSICS_COLLIDER_BOX ||
	    collider.kind > VOE_PHYSICS_COLLIDER_CAPSULE) {
		(void)snprintf(reason, size,
			       "unknown kind %" PRIu32 " kept, collides as nothing",
			       collider.kind);
		return SETTLED_WARNED;
	}
	return SETTLED_NOTHING;
}

// One line at the first settled intent of a run, and one closing the run at
// the first drain that settles none — scene/transform_system.h's report.
static void report(uint32_t settled_count, bool kept)
{
	if (settled_count > 0) {
		in_run = true;
		run_count += settled_count;
		run_kept = run_kept || kept;
		return;
	}
	if (!in_run)
		return;
	if (run_kept)
		VOE_BASE_ERROR("physics",
			       "voe_physics_collider: %" PRIu32
			       " intents settled in that run",
			       run_count);
	else
		VOE_BASE_WARNING("physics",
				 "voe_physics_collider: %" PRIu32
				 " intents settled in that run",
				 run_count);
	in_run = false;
	run_count = 0;
	run_kept = false;
}

void voe_physics_collider_system_run(voe_ecs_world *world)
{
	voe_ecs_intent queue;
	voe_ecs_type type;
	const voe_physics_collider_intent *intents;
	uint32_t count;
	uint32_t settled_count = 0;
	bool kept = false;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the collider system on no world");

	queue = voe_ecs_intent_type(world, &collider_intent_key);
	type = voe_ecs_component_type(world, &voe_physics_collider_key);
	intents = voe_ecs_intent_queue(world, queue);
	count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_entity entity = intents[i].entity;
		char reason[REASON];
		settling what;

		if (voe_ecs_component_get(world, type, entity) == NULL)
			continue;

		what = settled(intents[i].collider, reason, sizeof reason);
		if (what != SETTLED_NOTHING) {
			if (!in_run && settled_count == 0) {
				if (what == SETTLED_KEPT)
					VOE_BASE_ERROR("physics",
						       "voe_physics_collider: entity %" PRIu32 "v%" PRIu32 ": %s",
						       entity.index,
						       entity.generation, reason);
				else
					VOE_BASE_WARNING("physics",
							 "voe_physics_collider: entity %" PRIu32 "v%" PRIu32 ": %s",
							 entity.index,
							 entity.generation,
							 reason);
			}
			settled_count++;
		}
		if (what == SETTLED_KEPT) {
			kept = true;
			continue;
		}

		(void)voe_ecs_component_set(world, type, entity,
					    &intents[i].collider);
	}

	report(settled_count, kept);
	voe_ecs_intent_clear(world, queue);
}
