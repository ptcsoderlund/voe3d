// The sound and sound voice components: their keys, the registration, the
// reads and the two submits. The drains and the playing are the sound
// system's; see audio/sound_component.h.
//
// THE REPLACE INTENT'S KEY IS THIS FILE'S ALONE: the submit and the system find
// that queue as the sound's replace (ecs/component.h). The control's key is in
// the header, since nothing else names its queue.
#include <audio/sound_component.h>

#include <base/assert.h>

#include <ecs/intent.h>

#include <stddef.h>

const struct voe_ecs_key voe_audio_sound_key = { "voe_audio_sound" };
const struct voe_ecs_key voe_audio_sound_voice_key = { "voe_audio_sound_voice" };
const struct voe_ecs_key voe_audio_sound_control_key = {
	"voe_audio_sound_control"
};

static const struct voe_ecs_key sound_intent_key = { "voe_audio_sound_intent" };

// 0304 point 6's defaults: playing at once, so an authored sound is heard.
static const voe_audio_sound sound_default = {
	.path = "",
	.playing = true,
	.loop = false,
	.volume = 1.0f,
	.pitch = 1.0f,
};

static const voe_base_struct_description *sound_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_audio_sound_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

static voe_ecs_type sound_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_audio_sound_key);
}

static voe_ecs_type voice_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_audio_sound_voice_key);
}

void voe_audio_sound_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering sounds in no world");

	type = voe_ecs_component_register(world, &voe_audio_sound_key,
					  sizeof(voe_audio_sound), capacity,
					  sound_description());
	intent = voe_ecs_intent_register(world, &sound_intent_key,
					 sizeof(voe_audio_sound_intent), capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_audio_sound_intent, sound));
	voe_ecs_component_default_set(world, type, &sound_default);
	voe_ecs_component_menu_set(world, type, "Audio / Sound");

	(void)voe_ecs_intent_register(world, &voe_audio_sound_control_key,
				      sizeof(voe_audio_sound_control), capacity);
	(void)voe_ecs_component_register(world, &voe_audio_sound_voice_key,
					 sizeof(voe_audio_sound_voice), capacity,
					 &voe_ecs_runtime_only);
}

bool voe_audio_sound_add(voe_ecs_world *world, voe_ecs_entity entity,
			 voe_audio_sound sound)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a sound to no world");

	return voe_ecs_component_add(world, sound_type(world), entity, &sound);
}

const voe_audio_sound *voe_audio_sound_get(const voe_ecs_world *world,
					   voe_ecs_entity entity)
{
	return voe_ecs_component_get(world, sound_type(world), entity);
}

uint32_t voe_audio_sound_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(world, sound_type(world));
}

const voe_audio_sound *voe_audio_sound_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(world, sound_type(world));
}

const voe_ecs_entity *voe_audio_sound_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(world, sound_type(world));
}

const voe_audio_sound_voice *voe_audio_sound_voice_get(const voe_ecs_world *world,
						       voe_ecs_entity entity)
{
	return voe_ecs_component_get(world, voice_type(world), entity);
}

uint32_t voe_audio_sound_voice_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(world, voice_type(world));
}

const voe_audio_sound_voice *voe_audio_sound_voice_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(world, voice_type(world));
}

const voe_ecs_entity *voe_audio_sound_voice_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(world, voice_type(world));
}

bool voe_audio_sound_submit(voe_ecs_world *world, voe_audio_sound_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a sound to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_component_replace(world, sound_type(world)).intent,
		&intent);
}

bool voe_audio_sound_control_submit(voe_ecs_world *world,
				    voe_audio_sound_control control)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "controlling a sound in no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &voe_audio_sound_control_key),
		&control);
}
