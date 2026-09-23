// The camera system: registration, creation, and the drain that is the only
// thing in the engine that changes a lens.
//
// THE DRAIN REFUSES RATHER THAN CORRECTS. A lens has no nearest valid value the
// way a drifted rotation has a unit one: a near plane of nought could become
// anything above it, and guessing which would put a number in the row nobody
// asked for. So a refused lens keeps the last valid row, and says so once, on
// stderr, per intent.
//
// THE ENTITY IS NAMED BY ITS INDEX AND GENERATION. The transform's report looks
// an identity up for a name; a camera is refused rarely enough, and by a person
// typing into its one lens, that the number is enough to find it.
#include <base/assert.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <scene/camera_system.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>

// π as a float, the field of view's upper bound, which it may not reach.
#define HALF_TURN 3.14159265f

static const struct voe_ecs_key camera_intent_key = {
	"voe_scene_camera_intent"
};

static const voe_base_struct_description *camera_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_camera_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_scene_camera_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_type transform;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering cameras in no world");

	// Asserts when no transform was registered: a camera needs one.
	transform = voe_ecs_component_type(world, &voe_scene_transform_key);
	type = voe_ecs_component_register(world, &voe_scene_camera_key,
					  sizeof(voe_scene_camera), capacity,
					  camera_description());
	intent = voe_ecs_intent_register(world, &camera_intent_key,
					 sizeof(voe_scene_camera_intent),
					 capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_scene_camera_intent, camera));
	voe_ecs_component_default_set(
		world, type,
		&(voe_scene_camera){ .fov_y = 1.0471976f,
				     .near_plane = 0.1f,
				     .far_plane = 1000.0f });
	voe_ecs_component_needs_set(world, type, transform);
}

bool voe_scene_camera_add(voe_ecs_world *world, voe_ecs_entity entity,
			  voe_scene_camera camera)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a camera to no world");
	VOE_BASE_DEBUG_ASSERT(camera.fov_y > 0.0f,
			      "a camera with no field of view");
	VOE_BASE_DEBUG_ASSERT(camera.near_plane > 0.0f,
			      "a camera whose near plane is at or behind the eye");
	VOE_BASE_DEBUG_ASSERT(camera.far_plane > camera.near_plane,
			      "a camera whose far plane is not further away than its near one");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_camera_key),
		entity, &camera);
}

bool voe_scene_camera_submit(voe_ecs_world *world,
			     voe_scene_camera_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a lens to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &camera_intent_key), &intent);
}

// The field that makes this lens unable to project, or NULL when it can. The
// comparisons are written so a NaN fails each of them.
static const char *refused_field(voe_scene_camera camera)
{
	if (!(isfinite(camera.fov_y) && camera.fov_y > 0.0f &&
	      camera.fov_y < HALF_TURN))
		return "fov_y";
	if (!(isfinite(camera.near_plane) && camera.near_plane > 0.0f))
		return "near_plane";
	if (!(isfinite(camera.far_plane) &&
	      camera.far_plane > camera.near_plane))
		return "far_plane";
	return NULL;
}

void voe_scene_camera_system_run(voe_ecs_world *world)
{
	voe_ecs_type type;
	voe_ecs_intent intents;
	const voe_scene_camera_intent *queue;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the camera system on no world");

	type = voe_ecs_component_type(world, &voe_scene_camera_key);
	intents = voe_ecs_intent_type(world, &camera_intent_key);
	queue = voe_ecs_intent_queue(world, intents);
	count = voe_ecs_intent_count(world, intents);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_entity entity = queue[i].entity;
		const char *field;

		if (voe_ecs_component_get(world, type, entity) == NULL)
			continue;
		field = refused_field(queue[i].camera);
		if (field != NULL) {
			fprintf(stderr,
				"error: camera %uv%u: %s refused, lens kept\n",
				entity.index, entity.generation, field);
			continue;
		}
		(void)voe_ecs_component_set(world, type, entity,
					    &queue[i].camera);
	}

	voe_ecs_intent_clear(world, intents);
	VOE_BASE_DEBUG_ASSERT(voe_ecs_intent_count(world, intents) == 0,
			      "a drained camera queue still holds intents");
}
