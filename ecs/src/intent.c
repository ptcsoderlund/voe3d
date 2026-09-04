// The intent queues: register one, submit into it from anywhere, drain it in the
// one system that owns it.
//
// A QUEUE IS AN ARRAY AND A COUNT, AND SUBMITTING IS A COPY ONTO THE END. There
// is no read cursor and no partial drain, because an owner that applied half a
// queue and left the rest would have to remember how far it got, and the next
// thing to go wrong would be two owners disagreeing about that number. Read all
// of it, apply all of it, clear it.
//
// NOTHING IN HERE KNOWS WHAT AN INTENT MEANS. A queue is a size and a capacity;
// what the bytes are, and what applying one does, belongs to the system whose
// key registered it.
#include "world_internal.h"

#include <base/assert.h>

#include <string.h>

static struct voe_ecs_queue *queue_at(const voe_ecs_world *world,
				      voe_ecs_intent type)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "an intent queue in no world");
	VOE_BASE_ASSERT(type.value < world->queue_count,
			"an intent type this world never registered");

	return &((voe_ecs_world *)world)->queues[type.value];
}

voe_ecs_intent voe_ecs_intent_register(voe_ecs_world *world,
				       const struct voe_ecs_key *key,
				       size_t size, uint32_t capacity)
{
	struct voe_ecs_queue *queue;
	voe_ecs_intent type;

	VOE_BASE_ASSERT(world != NULL, "registering an intent in no world");
	VOE_BASE_ASSERT(key != NULL, "registering an intent with no key");
	VOE_BASE_ASSERT(size > 0, "registering an intent of no bytes");
	VOE_BASE_ASSERT(capacity > 0, "registering an intent with room for none");
	VOE_BASE_ASSERT(world->queue_count < world->queue_capacity,
			"registering more intent types than this world was made for");

	for (uint32_t i = 0; i < world->queue_count; i++)
		VOE_BASE_ASSERT(world->queues[i].key != key,
				"registering the same intent key twice");

	type.value = world->queue_count++;
	queue = &world->queues[type.value];
	queue->key = key;
	queue->size = size;
	queue->capacity = capacity;
	queue->count = 0;
	queue->values = voe_base_arena_push(world->arena,
					    (size_t)capacity * size);
	return type;
}

voe_ecs_intent voe_ecs_intent_type(const voe_ecs_world *world,
				   const struct voe_ecs_key *key)
{
	voe_ecs_intent type;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "looking up an intent in no world");
	VOE_BASE_DEBUG_ASSERT(key != NULL, "looking up an intent with no key");

	for (uint32_t i = 0; i < world->queue_count; i++) {
		if (world->queues[i].key == key) {
			type.value = i;
			return type;
		}
	}

	VOE_BASE_ASSERT(false,
			"asking for an intent type nothing registered — the module's _register was never called");
	type.value = 0;
	return type;
}

bool voe_ecs_intent_submit(voe_ecs_world *world, voe_ecs_intent type,
			   const void *value)
{
	struct voe_ecs_queue *queue = queue_at(world, type);

	VOE_BASE_DEBUG_ASSERT(value != NULL, "submitting an intent from nothing");

	if (queue->count == queue->capacity)
		return false;

	memcpy(queue->values + (size_t)queue->count * queue->size, value,
	       queue->size);
	queue->count++;
	return true;
}

uint32_t voe_ecs_intent_count(const voe_ecs_world *world, voe_ecs_intent type)
{
	return queue_at(world, type)->count;
}

const void *voe_ecs_intent_queue(const voe_ecs_world *world,
				 voe_ecs_intent type)
{
	return queue_at(world, type)->values;
}

void voe_ecs_intent_clear(voe_ecs_world *world, voe_ecs_intent type)
{
	queue_at(world, type)->count = 0;
}
