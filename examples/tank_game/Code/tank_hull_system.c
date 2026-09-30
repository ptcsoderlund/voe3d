// The tank hull system: the control row's drive and turn into a hull's
// motion. Teaches driving a thing by its own facing: the turn is about the
// world's up, and the drive is along the hull's own -Z after that turn, so
// forward is always forward. The hull moves `speed × drive` and turns
// `turn × turn` for the step's seconds, so a stick at rest stops it dead.
//
// A hull is a root and moves only its own row: the turret and the barrel
// are its children and ride on it by parenting (0271), so nothing here
// touches them. Each hull gets one transform intent a step, turn and drive
// together.
//
// The dust (0299 point 3): a hull with no emitter gets the enemy's tread dust
// queued onto it, added at run time and never saved, since agents do not edit
// tank_body.prefab. It plays while the drive is past 0.1 either way and stops
// otherwise, the control sent only on the change, read from its `playing`.
//
// Constraints: one control row drives every hull. Nothing with no control
// row (headless, or before its first step). A full transform queue leaves
// the rest of the hulls where they were this step.
#include "tank_control.h"
#include "tank_hull.h"

#include <3d/emitter_component.h>

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>
#include <game/world.h>

#include <math/double3.h>
#include <math/float3.h>
#include <math/quat.h>

#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <math.h>

#define TANK_HULL_RADIANS_PER_DEGREE (3.14159265358979323846f / 180.0f)

// The drive, either way, past which the treads kick up dust.
#define TANK_HULL_DUST_DRIVE 0.1f

const struct voe_ecs_key tank_hull_key = { "tank_hull" };

static const tank_hull tank_hull_default = { .speed = 4.0f, .turn = 90.0f };

bool tank_hull_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank hull in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_hull_key, sizeof(tank_hull), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(tank_hull), &tank_hull_default,
		"Tank / Hull", &voe_scene_transform_key });
}

// (0, 0, -1) rotated by the unit quaternion q: the third column of its
// matrix, negated. math/quat.h has no rotate, and this is all of it needed.
static voe_math_float3 forward_of(voe_math_quat q)
{
	const voe_math_float3 forward = {
		-2.0f * (q.x * q.z + q.w * q.y),
		-2.0f * (q.y * q.z - q.w * q.x),
		-(1.0f - 2.0f * (q.x * q.x + q.y * q.y)),
	};

	VOE_BASE_DEBUG_ASSERT(voe_math_float3_length(forward) < 1.01f,
			      "a forward longer than one");
	return forward;
}

// Queues the tread dust onto a hull with no emitter, as the lives system adds
// its collider (0299 point 3): enemy_tank.prefab's dust, not playing. With
// one, sends PLAY or STOP only when `moving` differs from its `playing`. A
// refused add or control is left for the next step.
static void dust(voe_ecs_world *world, voe_ecs_entity entity, bool moving)
{
	VOE_BASE_ASSERT(world != NULL, "dust in no world");
	const voe_3d_emitter *emitter = voe_3d_emitter_get(world, entity);

	if (emitter != NULL) {
		if (emitter->playing != moving)
			(void)voe_3d_emitter_control_submit(
				world, (voe_3d_emitter_control){
					.entity = entity,
					.kind = moving ? VOE_3D_EMITTER_PLAY :
							 VOE_3D_EMITTER_STOP });
		return;
	}
	const voe_3d_emitter row = {
		.playing = false,
		.rate = 12.0f,
		.life = 1.0f,
		.speed = 0.5f,
		.spread = 50.0f,
		.offset = { 0.0f, -1.7f, 1.4f },
		.direction = { 0.0f, 1.0f, 0.0f },
		.rise = 0.3f,
		.drag = 1.0f,
		.size_start = 0.4f,
		.size_end = 1.4f,
		.colour_start = { 0.45f, 0.35f, 0.22f },
		.colour_end = { 0.45f, 0.35f, 0.22f },
		.alpha_start = 0.5f,
		.alpha_end = 0.0f,
	};

	VOE_BASE_DEBUG_ASSERT(row.rate > 0.0f && row.life > 0.0f,
			      "dust that never shows");
	(void)voe_ecs_structure_add(
		world, voe_ecs_component_type(world, &voe_3d_emitter_key), entity,
		&row);
}

void tank_hull_system_run(voe_ecs_world *world, double seconds)
{
	VOE_BASE_ASSERT(world != NULL, "driving hulls in no world");
	VOE_BASE_ASSERT(seconds >= 0.0, "driving hulls back in time");
	const voe_ecs_type control_type =
		voe_ecs_component_type(world, &tank_control_key);

	if (voe_ecs_component_count(world, control_type) == 0)
		return;
	const tank_control *control =
		voe_ecs_component_rows(world, control_type);
	const float drive = control->drive;
	const float turn = control->turn;
	const bool moving = fabsf(drive) > TANK_HULL_DUST_DRIVE;
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_hull_key);
	const voe_ecs_type transform_type =
		voe_ecs_component_type(world, &voe_scene_transform_key);
	const tank_hull *rows = voe_ecs_component_rows(world, type);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more tank hull rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_transform *transform = voe_ecs_component_get(
			world, transform_type, entities[i]);

		if (transform == NULL)
			continue;
		dust(world, entities[i], moving);
		voe_scene_transform driven = *transform;

		driven.rotation = voe_math_quat_normalize(voe_math_quat_mul(
			voe_math_quat_from_axis_angle(
				(voe_math_float3){ 0.0f, 1.0f, 0.0f },
				turn * rows[i].turn *
					TANK_HULL_RADIANS_PER_DEGREE *
					(float)seconds),
			transform->rotation));
		driven.position = voe_math_double3_add(
			transform->position,
			voe_math_double3_from_float3(voe_math_float3_scale(
				forward_of(driven.rotation),
				drive * rows[i].speed * (float)seconds)));
		if (!voe_scene_transform_submit(world,
						(voe_scene_transform_intent){
							entities[i], driven }))
			return;
	}
}
