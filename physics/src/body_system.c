// The body system: registration, the one direct creation call, the drain that
// settles what it applies, and the report that settling writes. The move that
// writes velocity and on_floor from the world is card 13's and lands here.
//
// THE RUN FLAG AND THE COUNTERS ARE FILE-SCOPE STATICS AND THEREFORE PER
// PROCESS, as the collider's are; scene/transform_system.h says why that is
// enough.
#include <base/assert.h>
#include <base/report.h>
#include <ecs/intent.h>
#include <physics/body_system.h>
#include <physics/collider_component.h>

#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

static const struct voe_ecs_key body_intent_key = { "voe_physics_body_intent" };

// Long enough for the longest sentence below with three `%g` floats in it.
#define REASON 160

// A slope limit above this is no floor at all.
#define QUARTER_TURN 1.57079632679489661923f

static bool in_run;
static uint32_t run_count;

static const voe_base_struct_description *body_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_physics_body_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_physics_body_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_type collider;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering bodies in no world");

	// Asserts when no collider was registered: a body needs one.
	collider = voe_ecs_component_type(world, &voe_physics_collider_key);
	type = voe_ecs_component_register(world, &voe_physics_body_key,
					  sizeof(voe_physics_body), capacity,
					  body_description());
	intent = voe_ecs_intent_register(world, &body_intent_key,
					 sizeof(voe_physics_body_intent),
					 capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_physics_body_intent, body));
	voe_ecs_component_default_set(
		world, type,
		&(voe_physics_body){ .step_height = 0.3f,
				     .slope_limit = 0.8f,
				     .velocity = { 0.0f, 0.0f, 0.0f },
				     .on_floor = false });
	voe_ecs_component_needs_set(world, type, collider);
	voe_ecs_component_menu_set(world, type, "Physics / Kinematic Body");
}

bool voe_physics_body_add(voe_ecs_world *world, voe_ecs_entity entity,
			  voe_physics_body body)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a body to no world");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_physics_body_key),
		entity, &body);
}

bool voe_physics_body_submit(voe_ecs_world *world,
			     voe_physics_body_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a body to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &body_intent_key), &intent);
}

// True when the row may be applied; otherwise writes the sentence the report
// prints. Each test is written so a NaN fails it.
static bool usable(voe_physics_body body, char *reason, size_t size)
{
	voe_math_float3 v = body.velocity;

	if (!(isfinite(body.step_height) && body.step_height >= 0.0f)) {
		(void)snprintf(reason, size,
			       "step height %g is not finite or below 0, row kept",
			       (double)body.step_height);
		return false;
	}
	if (!(body.slope_limit >= 0.0f && body.slope_limit <= QUARTER_TURN)) {
		(void)snprintf(reason, size,
			       "slope limit %g is outside [0, pi/2], row kept",
			       (double)body.slope_limit);
		return false;
	}
	if (!(isfinite(v.x) && isfinite(v.y) && isfinite(v.z))) {
		(void)snprintf(reason, size,
			       "velocity (%g, %g, %g) is not finite, row kept",
			       (double)v.x, (double)v.y, (double)v.z);
		return false;
	}
	return true;
}

// One line at the first settled intent of a run, and one closing the run at
// the first drain that settles none — physics/collider_system.h's report, where
// every settling here is a kept row and so an error.
static void report(uint32_t settled_count)
{
	if (settled_count > 0) {
		in_run = true;
		run_count += settled_count;
		return;
	}
	if (!in_run)
		return;
	VOE_BASE_ERROR("physics",
		       "voe_physics_body: %" PRIu32 " intents settled in that run",
		       run_count);
	in_run = false;
	run_count = 0;
}

void voe_physics_body_system_run(voe_ecs_world *world)
{
	voe_ecs_intent queue;
	voe_ecs_type type;
	const voe_physics_body_intent *intents;
	uint32_t count;
	uint32_t settled_count = 0;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the body system on no world");

	queue = voe_ecs_intent_type(world, &body_intent_key);
	type = voe_ecs_component_type(world, &voe_physics_body_key);
	intents = voe_ecs_intent_queue(world, queue);
	count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_entity entity = intents[i].entity;
		char reason[REASON];

		if (voe_ecs_component_get(world, type, entity) == NULL)
			continue;
		if (!usable(intents[i].body, reason, sizeof reason)) {
			if (!in_run && settled_count == 0)
				VOE_BASE_ERROR("physics",
					       "voe_physics_body: entity %" PRIu32 "v%" PRIu32 ": %s",
					       entity.index, entity.generation,
					       reason);
			settled_count++;
			continue;
		}
		(void)voe_ecs_component_set(world, type, entity,
					    &intents[i].body);
	}

	report(settled_count);
	voe_ecs_intent_clear(world, queue);
}
