// The emitter and particles components: their keys, the registration, the
// reads and the two submits. The drains and the spawning are the emitter
// system's; see 3d/emitter_component.h.
//
// THE REPLACE INTENT'S KEY IS THIS FILE'S ALONE: the submit and the system find
// that queue as the emitter's replace (ecs/component.h). The control's key is in
// the header, since nothing else names its queue.
#include <3d/emitter_component.h>

#include <base/assert.h>

#include <ecs/intent.h>

#include <scene/transform_component.h>

#include <stddef.h>

const struct voe_ecs_key voe_3d_emitter_key = { "voe_3d_emitter" };
const struct voe_ecs_key voe_3d_particles_key = { "voe_3d_particles" };
const struct voe_ecs_key voe_3d_emitter_control_key = {
	"voe_3d_emitter_control"
};

static const struct voe_ecs_key emitter_intent_key = { "voe_3d_emitter_intent" };

// 0298 point 1's defaults: playing at once, so it is seen live in the editor.
static const voe_3d_emitter emitter_default = {
	.playing = true,
	.rate = 20.0f,
	.burst = 0,
	.life = 1.5f,
	.speed = 1.0f,
	.spread = 20.0f,
	.offset = { 0.0f, 0.0f, 0.0f },
	.direction = { 0.0f, 1.0f, 0.0f },
	.size_start = 0.3f,
	.size_end = 0.6f,
	.colour_start = { 1.0f, 1.0f, 1.0f },
	.colour_end = { 1.0f, 1.0f, 1.0f },
	.alpha_start = 1.0f,
	.alpha_end = 0.0f,
};

static const voe_base_struct_description *emitter_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_3d_emitter_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

static voe_ecs_type emitter_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_3d_emitter_key);
}

static voe_ecs_type particles_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_3d_particles_key);
}

void voe_3d_emitter_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering emitters in no world");

	type = voe_ecs_component_register(world, &voe_3d_emitter_key,
					  sizeof(voe_3d_emitter), capacity,
					  emitter_description());
	intent = voe_ecs_intent_register(world, &emitter_intent_key,
					 sizeof(voe_3d_emitter_intent), capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_3d_emitter_intent, emitter));
	voe_ecs_component_default_set(world, type, &emitter_default);
	voe_ecs_component_needs_set(
		world, type,
		voe_ecs_component_type(world, &voe_scene_transform_key));
	voe_ecs_component_menu_set(world, type, "Rendering / Particle emitter");

	(void)voe_ecs_intent_register(world, &voe_3d_emitter_control_key,
				      sizeof(voe_3d_emitter_control), capacity);
	(void)voe_ecs_component_register(world, &voe_3d_particles_key,
					 sizeof(voe_3d_particles), capacity,
					 &voe_ecs_runtime_only);
}

bool voe_3d_emitter_add(voe_ecs_world *world, voe_ecs_entity entity,
			voe_3d_emitter emitter)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding an emitter to no world");

	return voe_ecs_component_add(world, emitter_type(world), entity,
				     &emitter);
}

const voe_3d_emitter *voe_3d_emitter_get(const voe_ecs_world *world,
					 voe_ecs_entity entity)
{
	return voe_ecs_component_get(world, emitter_type(world), entity);
}

uint32_t voe_3d_emitter_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(world, emitter_type(world));
}

const voe_3d_emitter *voe_3d_emitter_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(world, emitter_type(world));
}

const voe_ecs_entity *voe_3d_emitter_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(world, emitter_type(world));
}

const voe_3d_particles *voe_3d_particles_get(const voe_ecs_world *world,
					     voe_ecs_entity entity)
{
	return voe_ecs_component_get(world, particles_type(world), entity);
}

uint32_t voe_3d_particles_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(world, particles_type(world));
}

const voe_3d_particles *voe_3d_particles_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(world, particles_type(world));
}

const voe_ecs_entity *voe_3d_particles_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(world, particles_type(world));
}

bool voe_3d_emitter_submit(voe_ecs_world *world, voe_3d_emitter_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting an emitter to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_component_replace(world, emitter_type(world)).intent,
		&intent);
}

bool voe_3d_emitter_control_submit(voe_ecs_world *world,
				   voe_3d_emitter_control control)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "controlling an emitter in no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &voe_3d_emitter_control_key),
		&control);
}
