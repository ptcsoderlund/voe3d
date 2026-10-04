// The light blocker system: registration, the one direct creation call, and
// the drain that applies a whole row or keeps the row over one it refuses.
//
// THE DRAIN IS THE WHOLE SYSTEM, as it is for the sun. What a box does to
// light is the renderer's, and where it is is its transform's.
//
// AN INTENT FOR AN ENTITY WITH NO BLOCKER IS DROPPED, silently — see the
// header. A refused size or kind says so once per intent on stderr, naming the entity
// by its index and generation as the sun's drain does.
#include <base/assert.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <scene/light_blocker_system.h>
#include <scene/transform_component.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>

// One intent per blocker per frame is the sizing this assumes, so the queue is
// as long as the table.
static const struct voe_ecs_key blocker_intent_key = {
	"voe_scene_light_blocker_intent"
};

static const voe_base_struct_description *blocker_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_light_blocker_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_scene_light_blocker_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_type transform;
	voe_ecs_intent intent;
	const voe_scene_light_blocker row = {
		.size = { 1.0f, 1.0f, 1.0f }, .kind = VOE_SCENE_LIGHT_BLOCKER_ROOM
	};

	VOE_BASE_ASSERT(world != NULL, "registering light blockers in no world");

	// Asserts when no transform was registered: a blocker is placed by one.
	transform = voe_ecs_component_type(world, &voe_scene_transform_key);
	type = voe_ecs_component_register(world, &voe_scene_light_blocker_key,
					  sizeof(voe_scene_light_blocker),
					  capacity, blocker_description());
	intent = voe_ecs_intent_register(world, &blocker_intent_key,
					 sizeof(voe_scene_light_blocker_intent),
					 capacity);
	voe_ecs_component_replace_set(
		world, type, intent,
		offsetof(voe_scene_light_blocker_intent, blocker));
	voe_ecs_component_default_set(world, type, &row);
	voe_ecs_component_needs_set(world, type, transform);
	voe_ecs_component_menu_set(world, type, "Rendering / Light blocker");
}

// Finite and not negative. Written so a NaN fails it.
static bool extent_valid(float value)
{
	return isfinite(value) && value >= 0.0f;
}

static bool size_valid(voe_math_float3 size)
{
	return extent_valid(size.x) && extent_valid(size.y) &&
	       extent_valid(size.z);
}

bool voe_scene_light_blocker_add(voe_ecs_world *world, voe_ecs_entity entity,
				 voe_scene_light_blocker blocker)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a light blocker to no world");
	VOE_BASE_DEBUG_ASSERT(size_valid(blocker.size),
			      "a light blocker size that is negative or not finite");
	VOE_BASE_DEBUG_ASSERT(blocker.kind <= VOE_SCENE_LIGHT_BLOCKER_WALL,
			      "a light blocker kind past Wall");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_light_blocker_key),
		entity, &blocker);
}

bool voe_scene_light_blocker_submit(voe_ecs_world *world,
				    voe_scene_light_blocker_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "submitting a light blocker to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &blocker_intent_key), &intent);
}

void voe_scene_light_blocker_system_run(voe_ecs_world *world)
{
	voe_ecs_intent queue;
	voe_ecs_type type;
	const voe_scene_light_blocker_intent *intents;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "running the light blocker system on no world");

	queue = voe_ecs_intent_type(world, &blocker_intent_key);
	type = voe_ecs_component_type(world, &voe_scene_light_blocker_key);
	intents = voe_ecs_intent_queue(world, queue);
	count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_entity entity = intents[i].entity;

		if (voe_ecs_component_get(world, type, entity) == NULL)
			continue;
		if (!size_valid(intents[i].blocker.size)) {
			fprintf(stderr,
				"error: light blocker %uv%u: size refused, row kept\n",
				entity.index, entity.generation);
			continue;
		}
		if (intents[i].blocker.kind > VOE_SCENE_LIGHT_BLOCKER_WALL) {
			fprintf(stderr,
				"error: light blocker %uv%u: kind %u refused, row kept\n",
				entity.index, entity.generation,
				intents[i].blocker.kind);
			continue;
		}
		(void)voe_ecs_component_set(world, type, entity,
					    &intents[i].blocker);
	}

	voe_ecs_intent_clear(world, queue);
}
