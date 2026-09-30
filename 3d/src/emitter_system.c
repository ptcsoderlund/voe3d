// The emitter system's run: the two drains, the particles rows added and
// dropped, the births and the step that moves and kills. See
// 3d/emitter_system.h for what a run does and in which order.
//
// A PARTICLES ROW IS CHANGED AS A COPY, set back whole: the table hands out no
// writable row. About two kilobytes an emitter a step, which has not mattered;
// a writable row in ecs would lift it.
//
// THE CORRECTED-TEXTURE RUN AND COUNT ARE FILE-SCOPE STATICS, per process, the
// trade 3d/src/model_component.c makes.
#include <3d/emitter_system.h>

#include <base/assert.h>
#include <base/report.h>

#include <ecs/component.h>
#include <ecs/intent.h>

#include <math/float4x4.h>

#include <scene/transform_component.h>

#include <inttypes.h>
#include <math.h>
#include <string.h>

#define PI 3.14159265358979f

static bool in_corrected_run;
static uint32_t corrected_run_count;

static voe_ecs_type emitter_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_3d_emitter_key);
}

static voe_ecs_type particles_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_3d_particles_key);
}

// Applies every replace in submission order, a texture with no end cut and
// reported as the model's path is.
static void drain_replaces(voe_ecs_world *world)
{
	voe_ecs_type type = emitter_type(world);
	voe_ecs_intent queue = voe_ecs_component_replace(world, type).intent;
	const voe_3d_emitter_intent *intents = voe_ecs_intent_queue(world, queue);
	uint32_t count = voe_ecs_intent_count(world, queue);
	uint32_t corrected = 0;

	for (uint32_t i = 0; i < count; i++) {
		voe_3d_emitter row = intents[i].emitter;

		if (voe_ecs_component_get(world, type, intents[i].entity) == NULL)
			continue;
		if (memchr(row.texture, '\0', VOE_3D_EMITTER_TEXTURE) == NULL) {
			row.texture[VOE_3D_EMITTER_TEXTURE - 1] = '\0';
			if (!in_corrected_run && corrected == 0)
				VOE_BASE_WARNING(
					"3d",
					"voe_3d_emitter: entity %" PRIu32 "v%" PRIu32
					": texture of %d bytes with no end, cut to \"%s\"",
					intents[i].entity.index,
					intents[i].entity.generation,
					VOE_3D_EMITTER_TEXTURE, row.texture);
			corrected++;
		}
		(void)voe_ecs_component_set(world, type, intents[i].entity, &row);
	}

	if (corrected > 0) {
		in_corrected_run = true;
		corrected_run_count += corrected;
	} else if (in_corrected_run) {
		VOE_BASE_WARNING("3d",
				 "voe_3d_emitter: %" PRIu32
				 " intents corrected in that run",
				 corrected_run_count);
		in_corrected_run = false;
		corrected_run_count = 0;
	}
	voe_ecs_intent_clear(world, queue);
}

// The entity's particles row, added when it has none: seeded from the entity,
// and holding the burst when the emitter plays. Asserts the table has room,
// since it is registered with the emitter's capacity.
static voe_3d_particles row_of(voe_ecs_world *world, voe_ecs_entity entity,
			       const voe_3d_emitter *emitter)
{
	const voe_3d_particles *own =
		voe_ecs_component_get(world, particles_type(world), entity);
	voe_3d_particles row = { 0 };

	if (own != NULL)
		return *own;

	row.seed = (entity.index + 1u) * 2654435761u ^ entity.generation * 40503u;
	if (row.seed == 0)
		row.seed = 0x9e3779b9u;
	row.burst = emitter->playing ? emitter->burst : 0;
	VOE_BASE_ASSERT(voe_ecs_component_add(world, particles_type(world),
					      entity, &row),
			"the particles table is smaller than the emitter table");
	return row;
}

// Applies every control in submission order to the emitter and its row.
static void drain_controls(voe_ecs_world *world)
{
	voe_ecs_type type = emitter_type(world);
	voe_ecs_intent queue =
		voe_ecs_intent_type(world, &voe_3d_emitter_control_key);
	const voe_3d_emitter_control *controls =
		voe_ecs_intent_queue(world, queue);
	uint32_t count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		const voe_3d_emitter *own =
			voe_ecs_component_get(world, type, controls[i].entity);
		voe_3d_emitter emitter;
		voe_3d_particles row;

		if (own == NULL)
			continue;
		emitter = *own;
		row = row_of(world, controls[i].entity, &emitter);
		if (controls[i].kind == VOE_3D_EMITTER_PLAY) {
			emitter.playing = true;
			row.burst += emitter.burst;
		} else if (controls[i].kind == VOE_3D_EMITTER_STOP) {
			emitter.playing = false;
		} else if (controls[i].kind == VOE_3D_EMITTER_BURST) {
			row.burst += controls[i].count != 0 ? controls[i].count :
							      emitter.burst;
		}
		(void)voe_ecs_component_set(world, type, controls[i].entity,
					    &emitter);
		(void)voe_ecs_component_set(world, particles_type(world),
					    controls[i].entity, &row);
	}
	voe_ecs_intent_clear(world, queue);
}

// Drops the rows whose emitter is gone, walking back so a swapped-in row has
// been seen, then adds one to each emitter lacking it.
static void add_and_drop_rows(voe_ecs_world *world)
{
	voe_ecs_type particles = particles_type(world);
	const voe_ecs_entity *owners = voe_ecs_component_entities(world, particles);
	const voe_3d_emitter *emitters = voe_3d_emitter_rows(world);
	const voe_ecs_entity *entities = voe_3d_emitter_entities(world);
	uint32_t emitter_count = voe_3d_emitter_count(world);

	for (uint32_t i = voe_ecs_component_count(world, particles); i > 0; i--)
		if (voe_3d_emitter_get(world, owners[i - 1]) == NULL)
			(void)voe_ecs_component_remove(world, particles,
						       owners[i - 1]);

	for (uint32_t i = 0; i < emitter_count; i++)
		(void)row_of(world, entities[i], &emitters[i]);
}

// The row's next number in [0, 1), from a 32-bit xorshift.
static float next_random(voe_3d_particles *row)
{
	uint32_t x = row->seed;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	row->seed = x;
	return (float)(x >> 8) * (1.0f / 16777216.0f);
}

// A unit direction within `spread` degrees of `about`, uniform over the cap;
// an `about` of no length, or NaN, is +Y.
static voe_math_float3 within_cone(voe_3d_particles *row, voe_math_float3 about,
				   float spread)
{
	float length = voe_math_float3_length(about);
	float half = fminf(fmaxf(spread, 0.0f), 180.0f) * (PI / 180.0f);
	float cos_t = 1.0f - next_random(row) * (1.0f - cosf(half));
	float sin_t = sqrtf(fmaxf(0.0f, 1.0f - cos_t * cos_t));
	float phi = 2.0f * PI * next_random(row);
	voe_math_float3 axis = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 side;
	voe_math_float3 other;

	if (length > 1e-6f)
		axis = voe_math_float3_scale(about, 1.0f / length);
	side = voe_math_float3_normalize(voe_math_float3_cross(
		axis, fabsf(axis.x) < 0.9f ? (voe_math_float3){ 1, 0, 0 } :
					     (voe_math_float3){ 0, 1, 0 }));
	other = voe_math_float3_cross(axis, side);
	return voe_math_float3_add(
		voe_math_float3_scale(axis, cos_t),
		voe_math_float3_scale(
			voe_math_float3_add(
				voe_math_float3_scale(side, cosf(phi)),
				voe_math_float3_scale(other, sinf(phi))),
			sin_t));
}

// Adds the owed and pending particles, up to the cap, at the emitter's place.
static void spawn(const voe_ecs_world *world, voe_ecs_entity entity,
		  const voe_3d_emitter *emitter, voe_3d_particles *row,
		  float seconds)
{
	float owed = 0.0f;
	voe_scene_transform place;
	voe_math_float4x4 matrix;
	voe_math_float4x4 turn;
	uint32_t births;

	if (emitter->playing) {
		row->debt += fmaxf(emitter->rate, 0.0f) * seconds;
		owed = fminf(floorf(row->debt), VOE_3D_EMITTER_PARTICLES);
		row->debt -= floorf(row->debt);
	} else {
		row->debt = 0.0f;
	}
	births = (uint32_t)owed + row->burst;
	row->burst = 0;
	if (births > VOE_3D_EMITTER_PARTICLES - row->count)
		births = VOE_3D_EMITTER_PARTICLES - row->count;
	if (births == 0 || voe_scene_transform_get(world, entity) == NULL)
		return;

	place = voe_scene_transform_world(world, entity);
	matrix = voe_scene_transform_matrix(place, place.position);
	turn = voe_math_float4x4_from_quat(place.rotation);
	for (uint32_t i = 0; i < births; i++) {
		voe_math_float3 way = voe_math_float4x4_transform_dir(
			turn, within_cone(row, emitter->direction,
					  emitter->spread));

		row->particles[row->count++] = (voe_3d_particle){
			.position = voe_math_double3_add(
				place.position,
				voe_math_double3_from_float3(
					voe_math_float4x4_transform_point(
						matrix, emitter->offset))),
			.velocity = voe_math_float3_scale(way, emitter->speed),
			.life = emitter->life,
		};
	}
}

// Moves and ages every live particle, removing each past its life by swapping
// the last one in.
static void integrate(const voe_3d_emitter *emitter, voe_3d_particles *row,
		      float seconds)
{
	float keep = fmaxf(0.0f, 1.0f - emitter->drag * seconds);
	uint32_t i = 0;

	while (i < row->count) {
		voe_3d_particle *particle = &row->particles[i];

		particle->velocity.y += emitter->rise * seconds;
		particle->velocity = voe_math_float3_scale(particle->velocity, keep);
		particle->position = voe_math_double3_add(
			particle->position,
			voe_math_double3_from_float3(voe_math_float3_scale(
				particle->velocity, seconds)));
		particle->age += seconds;
		if (particle->age >= particle->life)
			*particle = row->particles[--row->count];
		else
			i++;
	}
}

void voe_3d_emitter_system_run(voe_ecs_world *world, float seconds)
{
	voe_ecs_type particles;
	const voe_3d_emitter *emitters;
	const voe_ecs_entity *entities;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the emitter system on no world");
	VOE_BASE_ASSERT(seconds >= 0.0f, "the emitter system run backwards");

	drain_replaces(world);
	drain_controls(world);
	add_and_drop_rows(world);
	if (seconds == 0.0f)
		return;

	particles = particles_type(world);
	emitters = voe_3d_emitter_rows(world);
	entities = voe_3d_emitter_entities(world);
	count = voe_3d_emitter_count(world);
	for (uint32_t i = 0; i < count; i++) {
		voe_3d_particles row =
			*voe_3d_particles_get(world, entities[i]);

		spawn(world, entities[i], &emitters[i], &row, seconds);
		integrate(&emitters[i], &row, seconds);
		(void)voe_ecs_component_set(world, particles, entities[i], &row);
	}
}
