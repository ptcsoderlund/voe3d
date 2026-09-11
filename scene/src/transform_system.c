// The transform system: registration, the one direct creation call, and the
// drain.
//
// THE DRAIN IS THE WHOLE SYSTEM. It reads the queue in order, writes each row,
// and clears. There is nothing per-frame to compute, because a transform's
// matrix is built on demand by whoever wants it (transform_component.h) rather
// than cached here.
//
// AN INTENT FOR AN ENTITY WITH NO TRANSFORM IS DROPPED. voe_ecs_component_set
// returns false for a destroyed entity and for one that never had a transform,
// and both are ordinary rather than wrong — see the header.
//
// THE TRANSFORM REGISTERS ITS DESCRIPTION WHEN THE BUILD HAS ONE, AND NULL WHEN IT
// DOES NOT. The accessor only exists when descriptions are compiled in
// (base/describe.h), so the #if below is the condition that header tests. The
// table it returns is this file's own static copy and lives as long as the program.
#include <base/assert.h>
#include <ecs/intent.h>
#include <scene/transform_system.h>

#include <stddef.h>

// One intent per transform per frame is the sizing this assumes, so the queue is
// as long as the table.
static const struct voe_ecs_key transform_intent_key = {
	"voe_scene_transform_intent"
};

static const voe_base_struct_description *transform_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_transform_description();
#else
	return NULL;
#endif
}

void voe_scene_transform_register(voe_ecs_world *world, uint32_t capacity)
{
	VOE_BASE_ASSERT(world != NULL, "registering transforms in no world");

	(void)voe_ecs_component_register(world, &voe_scene_transform_key,
					 sizeof(voe_scene_transform), capacity,
					 transform_description());
	(void)voe_ecs_intent_register(world, &transform_intent_key,
				      sizeof(voe_scene_transform_intent),
				      capacity);
}

bool voe_scene_transform_add(voe_ecs_world *world, voe_ecs_entity entity,
			     voe_scene_transform transform)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a transform to no world");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_transform_key),
		entity, &transform);
}

bool voe_scene_transform_submit(voe_ecs_world *world,
				voe_scene_transform_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a transform to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &transform_intent_key),
		&intent);
}

void voe_scene_transform_system_run(voe_ecs_world *world)
{
	voe_ecs_intent queue;
	voe_ecs_type type;
	const voe_scene_transform_intent *intents;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the transform system on no world");

	queue = voe_ecs_intent_type(world, &transform_intent_key);
	type = voe_ecs_component_type(world, &voe_scene_transform_key);
	intents = voe_ecs_intent_queue(world, queue);
	count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++)
		(void)voe_ecs_component_set(world, type, intents[i].entity,
					    &intents[i].transform);

	voe_ecs_intent_clear(world, queue);
}
