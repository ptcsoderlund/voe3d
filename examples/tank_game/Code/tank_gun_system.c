// The tank gun system: fire held into spawned shots. Teaches spawning a
// cooked prefab from game code (0283 point 10): the shot is made at the
// barrel's world muzzle, turned as the barrel is, and lands at the step's
// structural apply.
//
// The shot's frame is the barrel: the gun's world rotation, every parent's
// included, turned about its own +Y by the `aim` of the turret on the same
// entity, or the gun's own frame when it has no turret. A model facing +Z
// needs an `aim` of 180, so the bare -Z would fire out of the back. The
// muzzle offset is turned by that frame and added to the gun's position, and
// the prefab's root is spawned there with it, so a shell flies along the
// barrel. It fires through tank_shell_fire, so the shot carries its tank_shot
// row (0294 point 1): `owner` the gun's topmost ancestor, `from` the gun's
// world position, not the muzzle. The gun's own
// `wait` row is the only thing written here, whole, through
// voe_ecs_component_set.
//
// The flash (0299 point 2): a gun with no emitter gets the enemy's muzzle
// flash queued onto it, added at run time and never saved, since agents do
// not edit tank_body.prefab; each shot fired bursts it. A gun with no point
// light gets the muzzle flash's light the same way, warm orange at the gun's
// own place, dark until each shot fired flashes it.
//
// The bang (0304 point 5): each shot fired plays `Assets/sounds/shot.wav` at
// the world muzzle through the step's mixer, nothing with no mixer (tests).
//
// Constraints: the control row's `fire` (tank_control.h), level not edge; at
// most one shot a gun a step, so a rate above the step rate fires at the step
// rate. Nothing with no control row (headless, or before its first step). A
// gun with no transform, a rate at or
// below zero or a prefab name with no NUL does not fire. A refused spawn
// leaves `wait` as it was, so the gun tries again next step.
#include "tank_control.h"
#include "tank_gun.h"
#include "tank_shell.h"
#include "tank_turret.h"

#include <3d/emitter_component.h>

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/world.h>

#include <math/double3.h>
#include <math/quat.h>

#include <scene/parent_component.h>
#include <scene/point_light_system.h>
#include <scene/transform_component.h>

#include <math.h>
#include <string.h>

#define TANK_GUN_RADIANS_PER_DEGREE (3.14159265358979323846f / 180.0f)

const struct voe_ecs_key tank_gun_key = { "tank_gun" };

static const tank_gun tank_gun_default = {
	.prefab = "shell",
	.rate = 6.0f,
	.muzzle = { 0.0f, 0.3f, -1.2f },
	.wait = 0.0f,
};

bool tank_gun_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank gun in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_gun_key, sizeof(tank_gun), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(tank_gun), &tank_gun_default,
		"Tank / Gun" }) &&
	       voe_game_project_component_needs(world, &tank_gun_key,
						&voe_scene_transform_key);
}

// v turned by the unit quaternion q: v + 2w(u x v) + 2u x (u x v).
// math/quat.h has no rotate, and this is all of it needed.
static voe_math_float3 turned_by(voe_math_quat q, voe_math_float3 v)
{
	const voe_math_float3 u = { q.x, q.y, q.z };
	const voe_math_float3 t =
		voe_math_float3_scale(voe_math_float3_cross(u, v), 2.0f);
	const voe_math_float3 out = voe_math_float3_add(
		voe_math_float3_add(v, voe_math_float3_scale(t, q.w)),
		voe_math_float3_cross(u, t));

	VOE_BASE_DEBUG_ASSERT(isfinite(out.x) && isfinite(out.z),
			      "a muzzle turned to nowhere");
	return out;
}

// The entity's topmost ancestor, itself when it has no parent: the walk
// ends at a missing, zeroed or dead parent, or VOE_SCENE_PARENT_DEPTH_MAX
// links, as scene/parent_component.h says.
static voe_ecs_entity root_of(const voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(world != NULL, "walking up in no world");
	voe_ecs_entity root = entity;

	for (int link = 0; link < VOE_SCENE_PARENT_DEPTH_MAX; link++) {
		const voe_scene_parent *parent = voe_scene_parent_get(world, root);

		if (parent == NULL || !voe_ecs_entity_alive(world, parent->parent))
			break;
		root = parent->parent;
	}
	VOE_BASE_DEBUG_ASSERT(voe_scene_parent_within(world, entity, root),
			      "a root not above its entity");
	return root;
}

// Spawns the gun's prefab at its world muzzle with its shot row. False when
// the spawn or the row is refused or the gun cannot fire.
static bool fire(const voe_game_project_step *step, voe_ecs_entity entity,
		 const tank_gun *gun)
{
	VOE_BASE_ASSERT(step != NULL && gun != NULL, "firing no gun");
	if (voe_scene_transform_get(step->world, entity) == NULL ||
	    memchr(gun->prefab, '\0', sizeof(gun->prefab)) == NULL)
		return false;
	const voe_scene_transform placed =
		voe_scene_transform_world(step->world, entity);
	const tank_turret *turret = voe_ecs_component_get(
		step->world,
		voe_ecs_component_type(step->world, &tank_turret_key), entity);
	const float aim = turret != NULL ? turret->aim : 0.0f;
	// The same product the turret aims with: world rotation, then aim.
	const voe_math_quat barrel = voe_math_quat_normalize(voe_math_quat_mul(
		placed.rotation,
		voe_math_quat_from_axis_angle(
			(voe_math_float3){ 0.0f, 1.0f, 0.0f },
			aim * TANK_GUN_RADIANS_PER_DEGREE)));
	const voe_math_double3 muzzle = voe_math_double3_add(
		placed.position,
		voe_math_double3_from_float3(turned_by(barrel, gun->muzzle)));

	if (!tank_shell_fire(step, gun->prefab, muzzle, barrel,
			     root_of(step->world, entity), placed.position))
		return false;
	if (step->audio != NULL)
		(void)voe_audio_mixer_play_at(step->audio, "Assets/sounds/shot.wav",
					      muzzle);
	return true;
}

// Queues the muzzle flash onto a gun with a transform and no emitter, as the
// lives system adds its collider (0299 point 2): enemy_tank.prefab's numbers,
// at the muzzle and along the barrel in the gun's own frame, so turned by its
// turret's `aim`. Never played; each shot bursts it. A refused add is left
// for the next step.
static void add_flash(voe_ecs_world *world, voe_ecs_entity entity,
		      const tank_gun *gun)
{
	VOE_BASE_ASSERT(world != NULL && gun != NULL, "a flash on no gun");
	if (voe_3d_emitter_get(world, entity) != NULL ||
	    voe_scene_transform_get(world, entity) == NULL)
		return;
	const tank_turret *turret = voe_ecs_component_get(
		world, voe_ecs_component_type(world, &tank_turret_key), entity);
	const voe_math_quat aim = voe_math_quat_from_axis_angle(
		(voe_math_float3){ 0.0f, 1.0f, 0.0f },
		(turret != NULL ? turret->aim : 0.0f) *
			TANK_GUN_RADIANS_PER_DEGREE);
	const voe_3d_emitter flash = {
		.playing = false,
		.burst = 12,
		.life = 0.12f,
		.speed = 2.0f,
		.spread = 30.0f,
		.offset = turned_by(aim, gun->muzzle),
		.direction = turned_by(aim, (voe_math_float3){ 0.0f, 0.0f, -1.0f }),
		.size_start = 0.9f,
		.size_end = 0.3f,
		.colour_start = { 1.0f, 0.9f, 0.7f },
		.colour_end = { 1.0f, 0.5f, 0.1f },
		.alpha_start = 1.0f,
		.alpha_end = 0.0f,
		.glow = 1.0f,
	};

	VOE_BASE_DEBUG_ASSERT(isfinite(flash.direction.x), "a flash nowhere");
	(void)voe_ecs_structure_add(
		world, voe_ecs_component_type(world, &voe_3d_emitter_key), entity,
		&flash);
}

// Queues the muzzle flash's light onto a gun with a transform and no point
// light, as add_flash does its emitter: enemy_tank.prefab's turret numbers,
// dark until a shot flashes it. A refused add is left for the next step.
static void add_flash_light(voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(world != NULL, "a flash light on no gun");
	if (voe_scene_point_light_get(world, entity) != NULL ||
	    voe_scene_transform_get(world, entity) == NULL)
		return;
	const voe_scene_point_light light = {
		.colour = { 1.0f, 0.7f, 0.35f },
		.intensity = 4.0f,
		.range = 6.0f,
		.flash = 0.12f,
		.flash_when_made = false,
	};

	VOE_BASE_DEBUG_ASSERT(light.range > 0.0f, "a flash light reaching nothing");
	(void)voe_ecs_structure_add(
		world, voe_ecs_component_type(world, &voe_scene_point_light_key),
		entity, &light);
}

// Bursts the gun's flash for a shot; nothing when it has none yet. A full
// control queue loses this one flash, not the shot.
static void burst_flash(voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(world != NULL, "flashing in no world");
	if (voe_3d_emitter_get(world, entity) == NULL)
		return;
	(void)voe_3d_emitter_control_submit(world, (voe_3d_emitter_control){
		.entity = entity, .kind = VOE_3D_EMITTER_BURST, .count = 0 });
}

// Flashes the gun's light for a shot; nothing when it has none yet. A full
// flash queue loses this one flash, not the shot.
static void flash_light(voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(world != NULL, "flashing a light in no world");
	if (voe_scene_point_light_get(world, entity) == NULL)
		return;
	(void)voe_scene_point_light_flash_submit(world, entity);
}

void tank_gun_system_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"firing guns in no world");
	VOE_BASE_ASSERT(step->seconds >= 0.0, "firing guns back in time");
	const voe_ecs_type control_type =
		voe_ecs_component_type(step->world, &tank_control_key);

	if (voe_ecs_component_count(step->world, control_type) == 0)
		return;
	const tank_control *control =
		voe_ecs_component_rows(step->world, control_type);
	const bool held = control->fire;
	const voe_ecs_type type =
		voe_ecs_component_type(step->world, &tank_gun_key);
	const tank_gun *rows = voe_ecs_component_rows(step->world, type);
	const voe_ecs_entity *entities =
		voe_ecs_component_entities(step->world, type);
	const uint32_t count = voe_ecs_component_count(step->world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more tank gun rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		tank_gun next = rows[i];

		add_flash(step->world, entities[i], &next);
		add_flash_light(step->world, entities[i]);
		next.wait -= (float)step->seconds;
		if (held && next.wait <= 0.0f && next.rate > 0.0f &&
		    fire(step, entities[i], &next)) {
			next.wait = 1.0f / next.rate;
			burst_flash(step->world, entities[i]);
			flash_light(step->world, entities[i]);
		}
		const bool ok = voe_ecs_component_set(step->world, type,
						      entities[i], &next);

		VOE_BASE_ASSERT(ok, "a tank gun row vanished while stepping it");
	}
}
