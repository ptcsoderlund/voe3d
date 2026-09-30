// A particle emitter an entity carries, its two intents, and the particles it
// has spawned. Read by anyone, const; written through the submits here and the
// emitter system (card 03 of 042), which is the one place rows change.
//
//     voe_scene_transform_register(world, capacity);   // first: an emitter needs one
//     voe_3d_emitter_register(world, capacity);
//     if (!voe_3d_emitter_control_submit(world, (voe_3d_emitter_control){
//                 .entity = entity, .kind = VOE_3D_EMITTER_BURST, .count = 8 }))
//             ...                                       // the queue is full
//
// AN EMITTER SAYS HOW PARTICLES ARE BORN AND AGE (0298 point 1): whether it
// plays, how many a second, a burst fired when it starts playing, each one's
// life, speed and spread (degrees, a cone about `direction`), where it is born
// and which way it goes (offset and direction, in the entity's local space),
// rise along world up and drag, and size, colour and alpha from birth to death,
// with a glow. It sits at "Rendering / Particle emitter" and needs a transform.
//
// THE DEFAULT ROW PLAYS: 20 a second, 1.5 s, 1 m/s, 20 degrees about +Y, 0.3 to
// 0.6 m, white, alpha 1 to 0, no burst, rise, drag or glow. It plays at once so
// an emitter added in the editor is seen live, with no step to start it.
//
// THE CONTROL STARTS AND STOPS SPAWNING, NOT THE PARTICLES: stop ends births and
// the live ones finish their life; a burst of count 0 is the row's burst. The
// replace intent is the whole row, as the model's is (3d/model_component.h).
//
// THE PARTICLES ARE A RUNTIME-ONLY ROW, one per emitter, never saved: up to
// VOE_3D_EMITTER_PARTICLES in world space, so they do not follow the emitter
// after birth, with the spawn debt, a pending burst and a private xorshift seed.
//
// THE TEXTURE IS A PROJECT-RELATIVE .png OR .jpg PATH, NUL-terminated in its 128
// bytes, that the model store loads (3d/models.h); empty is the built-in dot.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/double3.h>
#include <math/float3.h>

#include <stdint.h>

// The texture path's bytes, terminating NUL included.
#define VOE_3D_EMITTER_TEXTURE 128

// The most live particles one emitter has.
#define VOE_3D_EMITTER_PARTICLES 64

#define VOE_3D_EMITTER_FIELDS(F, F_READ_ONLY)        \
	F(bool, playing, BOOL)                       \
	F(float, rate, FLOAT32)                      \
	F(uint32_t, burst, UINT32)                   \
	F(float, life, FLOAT32)                      \
	F(float, speed, FLOAT32)                     \
	F(float, spread, FLOAT32)                    \
	F(voe_math_float3, offset, FLOAT3)           \
	F(voe_math_float3, direction, FLOAT3)        \
	F(float, rise, FLOAT32)                      \
	F(float, drag, FLOAT32)                      \
	F(float, size_start, FLOAT32)                \
	F(float, size_end, FLOAT32)                  \
	F(voe_math_float3, colour_start, COLOUR)     \
	F(voe_math_float3, colour_end, COLOUR)       \
	F(float, alpha_start, FLOAT32)               \
	F(float, alpha_end, FLOAT32)                 \
	F(float, glow, FLOAT32)                      \
	F(char, texture, CHAR, VOE_3D_EMITTER_TEXTURE)

VOE_BASE_DESCRIBE_STRUCT(voe_3d_emitter, VOE_3D_EMITTER_FIELDS)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_3d_emitter_key;

// Change this entity's emitter to the submitter's row.
typedef struct {
	voe_ecs_entity entity;
	voe_3d_emitter emitter;
} voe_3d_emitter_intent;

// What a control asks of an emitter.
typedef enum {
	VOE_3D_EMITTER_PLAY = 0,
	VOE_3D_EMITTER_STOP,
	VOE_3D_EMITTER_BURST,
} voe_3d_emitter_control_kind;

// Play, stop, or burst `count` particles (0 is the row's burst).
typedef struct {
	voe_ecs_entity entity;
	voe_3d_emitter_control_kind kind;
	uint32_t count;
} voe_3d_emitter_control;

// The control's queue key, for the emitter system that drains it.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_3d_emitter_control_key;

// One live particle, in world space, seconds for age and life.
typedef struct {
	voe_math_double3 position;
	voe_math_float3 velocity;
	float age;
	float life;
} voe_3d_particle;

// An emitter's live particles, the first `count` of `particles`; `debt` is the
// part of a particle owed to the next step, `burst` the one pending.
typedef struct {
	voe_3d_particle particles[VOE_3D_EMITTER_PARTICLES];
	uint32_t count;
	float debt;
	uint32_t burst;
	uint32_t seed;
} voe_3d_particles;

extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_3d_particles_key;

// Registers the emitter (its default row, its need of a transform, its menu
// path and its intent as its replace), the control intent, and the particles as
// runtime-only, all with room for `capacity`. Once per world, after transforms.
void voe_3d_emitter_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its emitter. False when the table is full or the entity is
// not alive. A direct call, as the model's: an intent changes one that exists.
[[nodiscard]] bool voe_3d_emitter_add(voe_ecs_world *world,
				      voe_ecs_entity entity,
				      voe_3d_emitter emitter);

// NULL when the entity has no emitter or is not alive.
const voe_3d_emitter *voe_3d_emitter_get(const voe_ecs_world *world,
					 voe_ecs_entity entity);

// The table. rows[i] belongs to entities[i], and both are `count` long.
uint32_t voe_3d_emitter_count(const voe_ecs_world *world);
const voe_3d_emitter *voe_3d_emitter_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_3d_emitter_entities(const voe_ecs_world *world);

// NULL when the entity has no particles row or is not alive.
const voe_3d_particles *voe_3d_particles_get(const voe_ecs_world *world,
					     voe_ecs_entity entity);

// The particles table, as the emitter's.
uint32_t voe_3d_particles_count(const voe_ecs_world *world);
const voe_3d_particles *voe_3d_particles_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_3d_particles_entities(const voe_ecs_world *world);

// False when the queue is full: the system has not run for long enough.
[[nodiscard]] bool voe_3d_emitter_submit(voe_ecs_world *world,
					 voe_3d_emitter_intent intent);
[[nodiscard]] bool voe_3d_emitter_control_submit(voe_ecs_world *world,
						 voe_3d_emitter_control control);
