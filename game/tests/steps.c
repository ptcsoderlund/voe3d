// The fixed steps on a game world: one step's time runs one step and draws a
// whole step behind, two and a half run two and draw half behind, a second
// runs the four at most and keeps under one step, and a capsule over a box
// floor, pulled down by the test's systems, lands and stands after 60 steps,
// and a follower set to a falling body's position after the move is where
// the body is by the end of the same step.
// Needs no window and no graphics card: nothing is shaped, so the shapes are
// zeros.
#include <game/steps.h>

#include <game/world.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <physics/body_component.h>
#include <physics/body_system.h>
#include <physics/collider_component.h>
#include <physics/collider_system.h>

#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#define GRAVITY 9.81f

// What the stub systems saw, the body they pull down when `falling`, and the
// entity set to the body's position after the move when `following`.
static struct {
	unsigned calls;
	unsigned after_calls;
	bool falling;
	bool following;
	voe_ecs_entity body;
	voe_ecs_entity follower;
} seen;

// Counts its calls; when falling, adds a step of gravity to the body's
// velocity, from rest when it is on the floor, as a project would (0249).
static void systems(const voe_game_project_step *step)
{
	voe_physics_body row;
	float fall = GRAVITY * (float)step->seconds;

	VOE_TEST_CHECK(step->seconds == VOE_GAME_STEP_SECONDS);
	seen.calls++;
	if (!seen.falling)
		return;
	row = *voe_physics_body_get(step->world, seen.body);
	row.velocity.y = row.on_floor ? -fall : row.velocity.y - fall;
	VOE_TEST_CHECK(voe_physics_body_submit(
		step->world, (voe_physics_body_intent){ seen.body, row }));
}

// Counts its calls; when following, submits the follower's transform with
// the body's current position, as a camera follow would (0256).
static void after_move(const voe_game_project_step *step)
{
	voe_scene_transform moved;

	VOE_TEST_CHECK(step->seconds == VOE_GAME_STEP_SECONDS);
	seen.after_calls++;
	if (!seen.following)
		return;
	moved = *voe_scene_transform_get(step->world, seen.follower);
	moved.position =
		voe_scene_transform_get(step->world, seen.body)->position;
	VOE_TEST_CHECK(voe_scene_transform_submit(
		step->world,
		(voe_scene_transform_intent){ seen.follower, moved }));
}

// An entity at `at` with a collider of `kind` and `size`.
static voe_ecs_entity placed(voe_ecs_world *world, voe_math_double3 at,
			     uint32_t kind, voe_math_float3 size)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = at,
				       .rotation = { 0, 0, 0, 1 },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_physics_collider_add(
		world, entity,
		(voe_physics_collider){ .kind = kind, .size = size }));
	return entity;
}

static void steps_are_counted(voe_ecs_world *world,
			      const voe_3d_shapes *shapes)
{
	voe_game_steps steps = { 0 };
	float lag;

	lag = voe_game_steps_run(&steps, world, NULL, shapes,
				 VOE_GAME_STEP_SECONDS, systems, after_move);
	VOE_TEST_CHECK_INT(seen.calls, 1);
	VOE_TEST_CHECK_FLOAT(lag, 1.0f, 0.0f);

	seen.calls = 0;
	steps = (voe_game_steps){ 0 };
	lag = voe_game_steps_run(&steps, world, NULL, shapes,
				 2.5 * VOE_GAME_STEP_SECONDS, systems, after_move);
	VOE_TEST_CHECK_INT(seen.calls, 2);
	VOE_TEST_CHECK_FLOAT(lag, 0.5f, 1e-4f);

	seen.calls = 0;
	steps = (voe_game_steps){ 0 };
	lag = voe_game_steps_run(&steps, world, NULL, shapes, 1.0, systems, after_move);
	VOE_TEST_CHECK_INT(seen.calls, VOE_GAME_STEPS_MAX);
	VOE_TEST_CHECK(steps.banked >= 0.0 &&
		       steps.banked < VOE_GAME_STEP_SECONDS);
	VOE_TEST_CHECK(lag > 0.0f && lag <= 1.0f);
}

// A capsule half a metre over a box floor whose top is y = 0.
static void a_body_lands(voe_ecs_world *world, const voe_3d_shapes *shapes)
{
	voe_game_steps steps = { 0 };

	(void)placed(world, (voe_math_double3){ 0.0, -0.5, 0.0 },
		     VOE_PHYSICS_COLLIDER_BOX,
		     (voe_math_float3){ 40.0f, 1.0f, 40.0f });
	seen.body = placed(world, (voe_math_double3){ 0.0, 1.5, 0.0 },
			   VOE_PHYSICS_COLLIDER_CAPSULE,
			   (voe_math_float3){ 1.0f, 2.0f, 1.0f });
	VOE_TEST_CHECK(voe_physics_body_add(
		world, seen.body,
		(voe_physics_body){ .step_height = 0.3f,
				    .slope_limit = 0.8f }));
	seen.falling = true;
	seen.calls = 0;
	for (int i = 0; i < 60; i++)
		(void)voe_game_steps_run(&steps, world, NULL, shapes,
					 VOE_GAME_STEP_SECONDS, systems, after_move);
	VOE_TEST_CHECK_INT(seen.calls, 60);
	VOE_TEST_CHECK(voe_physics_body_get(world, seen.body)->on_floor);
	VOE_TEST_CHECK_FLOAT(
		(float)voe_scene_transform_get(world, seen.body)->position.y,
		1.0f, 1e-3f);
}

// A capsule in the air far off the floor, and a follower at the origin.
static void a_follower_keeps_up(voe_ecs_world *world,
				const voe_3d_shapes *shapes)
{
	voe_game_steps steps = { 0 };
	voe_math_double3 body_at;
	voe_math_double3 follower_at;

	seen.body = placed(world, (voe_math_double3){ 100.0, 10.0, 0.0 },
			   VOE_PHYSICS_COLLIDER_CAPSULE,
			   (voe_math_float3){ 1.0f, 2.0f, 1.0f });
	VOE_TEST_CHECK(voe_physics_body_add(
		world, seen.body,
		(voe_physics_body){ .step_height = 0.3f,
				    .slope_limit = 0.8f }));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &seen.follower));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, seen.follower,
		(voe_scene_transform){ .rotation = { 0, 0, 0, 1 },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	seen.falling = true;
	seen.following = true;
	seen.calls = 0;
	seen.after_calls = 0;
	(void)voe_game_steps_run(&steps, world, NULL, shapes,
				 VOE_GAME_STEP_SECONDS, systems, after_move);
	VOE_TEST_CHECK_INT(seen.calls, 1);
	VOE_TEST_CHECK_INT(seen.after_calls, 1);
	body_at = voe_scene_transform_get(world, seen.body)->position;
	follower_at = voe_scene_transform_get(world, seen.follower)->position;
	VOE_TEST_CHECK(body_at.y < 10.0);
	VOE_TEST_CHECK(follower_at.x == body_at.x &&
		       follower_at.y == body_at.y &&
		       follower_at.z == body_at.z);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 22);
	voe_ecs_world *world = voe_game_world_new(arena);
	const voe_3d_shapes shapes = { 0 };

	steps_are_counted(world, &shapes);
	a_body_lands(world, &shapes);
	a_follower_keeps_up(world, &shapes);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
