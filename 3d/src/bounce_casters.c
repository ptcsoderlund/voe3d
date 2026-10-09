// A removed caster's stale sphere, from the memory the caller keeps per target
// (0394). The contract is draw_bounce.h's voe_3d_bounce_removed; the memory
// is 3d/bounce_casters.h's. voe_3d_draw_bounce calls it after the stale
// spheres, in the room they leave.
//
// A CASTER IS WHAT voe_3d_bounce_walk VISITS, the walk voe_3d_bounce_stale
// marks over, and its centre and radius are voe_3d_bounce_caster_sphere's at
// lag 0, so the memory and the stale spheres never disagree about what casts
// or how big it is. One no longer visited (dead, without its mesh or model
// row, hidden or no longer casting) marks where it was remembered, then the
// memory holds this frame's casters.
//
// Per frame (0388): a walk over the casters and one over the memory, on the
// CPU, inside the shadows call, every frame a light bounces; no GPU pass.
//
// Constraints: the new memory is built on the stack, 20 KB, because the
// shadows call takes no arena. Each remembered entry is looked for among
// this frame's casters linearly, at worst 512 × 512 comparisons a frame; a
// sort by entity index would lift it. A caster of no size is not remembered,
// and one whose sphere finds no room is forgotten unmarked.
#include "draw_bounce.h"
#include "draw_group.h"

#include <base/assert.h>

// One call's remembering: what it measures with, and this frame's casters.
struct remembering {
	const voe_ecs_world *world;
	const voe_render_device *device;
	const voe_3d_models *models;
	voe_3d_bounce_casters now;
};

// Caster `entity` into the remembering `context` when it has a size. Whether
// there is room for another.
static bool remember(void *context, voe_ecs_entity entity,
		     voe_render_geometry geometry, const voe_3d_model_entry *model)
{
	struct remembering *call = context;
	voe_math_double3 centre;
	float radius;

	VOE_BASE_ASSERT(call->now.count < VOE_3D_BOUNCE_CASTERS,
			"remembering past the memory's room");
	radius = voe_3d_bounce_caster_sphere(call->world, call->device, entity,
					     geometry, call->models, model,
					     0.0f, &centre);
	if (radius > 0.0f)
		call->now.entries[call->now.count++] =
			(voe_3d_bounce_caster){ entity, centre, radius };
	return call->now.count < VOE_3D_BOUNCE_CASTERS;
}

// Whether `entity` is among `now`'s casters.
static bool still_casts(const voe_3d_bounce_casters *now, voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(now->count <= VOE_3D_BOUNCE_CASTERS,
			"a memory past its room");
	for (uint32_t i = 0; i < now->count; i++)
		if (voe_3d_draw_group_is_the_same_entity(now->entries[i].entity,
							 entity))
			return true;
	return false;
}

uint32_t voe_3d_bounce_removed(const voe_ecs_world *world,
			       const voe_render_device *device,
			       const voe_3d_frame *frame,
			       voe_math_float4 *spheres, uint32_t room)
{
	struct remembering call = { world, device, frame->models, { 0 } };
	voe_3d_bounce_casters *memory;
	uint32_t count = 0;

	VOE_BASE_ASSERT(world != NULL && device != NULL && frame != NULL &&
				(spheres != NULL || room == 0),
			"marking removed casters with no world, device, frame or room");
	memory = frame->casters;
	if (memory == NULL)
		return 0;
	VOE_BASE_ASSERT(memory->count <= VOE_3D_BOUNCE_CASTERS,
			"a memory past its room");
	voe_3d_bounce_walk(world, frame, remember, &call);
	for (uint32_t i = 0; i < memory->count && count < room; i++) {
		const voe_3d_bounce_caster *was = &memory->entries[i];

		if (still_casts(&call.now, was->entity))
			continue;
		spheres[count++] = (voe_math_float4){
			(float)(was->centre.x - frame->eye.x),
			(float)(was->centre.y - frame->eye.y),
			(float)(was->centre.z - frame->eye.z), was->radius
		};
	}
	*memory = call.now;
	VOE_BASE_ASSERT(count <= room, "more removed spheres than room");
	return count;
}
