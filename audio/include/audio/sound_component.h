// A sound an entity carries, its two intents, and the voice that plays it.
// Read by anyone, const; written through the submits here and the sound system
// (card 05 of 044), which is the one place rows change.
//
//     voe_audio_sound_register(world, capacity);
//     if (!voe_audio_sound_control_submit(world, (voe_audio_sound_control){
//                 .entity = entity, .kind = VOE_AUDIO_SOUND_PLAY }))
//             ...                                       // the queue is full
//
// A SOUND IS A PATH PLAYED FROM A THING (0304 point 6): the clip, whether it
// plays, whether it loops, its volume and its pitch. It sits at "Audio / Sound"
// and needs nothing. A row whose entity has a transform is placed at its world
// position and follows it; one without plays unplaced, as the coin does.
//
// THE DEFAULT ROW PLAYS: empty path, playing, no loop, volume 1, pitch 1. It
// plays at once so a sound authored in the editor is heard with no step to
// start it; the empty path is silence until one is set.
//
// THE CONTROL: play starts from the beginning, again if it already plays; stop
// ends it; tune sets volume and pitch. The replace intent is the whole row, as
// the model's is (3d/model_component.h).
//
// THE VOICE IS A RUNTIME-ONLY ROW, one per sound, never saved: the mixer's
// handle (audio/mixer.h) and a restart flag a play leaves for the system.
//
// THE PATH IS PROJECT-RELATIVE, NUL-terminated in its VOE_AUDIO_SOUND_PATH
// bytes, read by the mixer from its folder.
#pragma once

#include <audio/mixer.h>

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <stdint.h>

// The path's bytes, terminating NUL included.
#define VOE_AUDIO_SOUND_PATH 128

#define VOE_AUDIO_SOUND_FIELDS(F, F_READ_ONLY)       \
	F(char, path, CHAR, VOE_AUDIO_SOUND_PATH)    \
	F(bool, playing, BOOL)                       \
	F(bool, loop, BOOL)                          \
	F(float, volume, FLOAT32)                    \
	F(float, pitch, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(voe_audio_sound, VOE_AUDIO_SOUND_FIELDS)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_audio_sound_key;

// Change this entity's sound to the submitter's row.
typedef struct {
	voe_ecs_entity entity;
	voe_audio_sound sound;
} voe_audio_sound_intent;

// What a control asks of a sound.
typedef enum {
	VOE_AUDIO_SOUND_PLAY = 0,
	VOE_AUDIO_SOUND_STOP,
	VOE_AUDIO_SOUND_TUNE,
} voe_audio_sound_control_kind;

// Play, stop, or tune to `volume` and `pitch` (read only by a tune).
typedef struct {
	voe_ecs_entity entity;
	voe_audio_sound_control_kind kind;
	float volume;
	float pitch;
} voe_audio_sound_control;

// The control's queue key, for the sound system that drains it.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_audio_sound_control_key;

// A sound's voice: the mixer's handle, 0 none, and whether the next run
// restarts it from the beginning.
typedef struct {
	voe_audio_voice voice;
	bool restart;
} voe_audio_sound_voice;

extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_audio_sound_voice_key;

// Registers the sound (its default row, its menu path and its intent as its
// replace), the control intent, and the voice as runtime-only, all with room
// for `capacity`. Once per world.
void voe_audio_sound_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its sound. False when the table is full or the entity is
// not alive. A direct call, as the model's: an intent changes one that exists.
[[nodiscard]] bool voe_audio_sound_add(voe_ecs_world *world,
				       voe_ecs_entity entity,
				       voe_audio_sound sound);

// NULL when the entity has no sound or is not alive.
const voe_audio_sound *voe_audio_sound_get(const voe_ecs_world *world,
					   voe_ecs_entity entity);

// The table. rows[i] belongs to entities[i], and both are `count` long.
uint32_t voe_audio_sound_count(const voe_ecs_world *world);
const voe_audio_sound *voe_audio_sound_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_audio_sound_entities(const voe_ecs_world *world);

// NULL when the entity has no voice row or is not alive.
const voe_audio_sound_voice *voe_audio_sound_voice_get(const voe_ecs_world *world,
						       voe_ecs_entity entity);

// The voice table, as the sound's.
uint32_t voe_audio_sound_voice_count(const voe_ecs_world *world);
const voe_audio_sound_voice *voe_audio_sound_voice_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_audio_sound_voice_entities(const voe_ecs_world *world);

// False when the queue is full: the system has not run for long enough.
[[nodiscard]] bool voe_audio_sound_submit(voe_ecs_world *world,
					  voe_audio_sound_intent intent);
[[nodiscard]] bool voe_audio_sound_control_submit(voe_ecs_world *world,
						  voe_audio_sound_control control);
