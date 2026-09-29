// The move of every kinematic body (0253 point 4): substeps, push-out, the
// step up and staying on the floor. It reads the world, writes only the body
// table's rows and moves a body only through a transform intent (0249 rule 2).
//
// A POSE IS A SHAPE, A VELOCITY AND TWO FLAGS, passed by value: every stage
// takes one and returns the next, so a step up is tried on a copy and kept or
// thrown away whole.
//
// POSITIONS ARE DOUBLE, MOTION IS FLOAT (0250): a shift adds a small float
// motion to the shape's double centre, and the overlap query works about it.
//
// A LEDGE IS TOLD FROM A SLOPE BY A PROBE. A round body meets a step's edge at
// the same steep normal a ramp gives it, so while settling after a step up a
// steep contact is floor only when a small sphere just inside and above the
// contact point touches nothing: a flat top, not a rising face.
//
// A BODY STANDS ON WORLD Y: its capsule's lower cap is taken to be straight
// below its centre, as a player's is. A body tipped over steps wrongly.
//
// A BODY IS EXPECTED TO BE A ROOT: its move is written as its row, so under a
// parent the world place it lands at is not the one it moved to.
//
// MOST_SUBSTEPS IS A CEILING: a body faster than MOST_SUBSTEPS quarter radii a
// call moves further than that per substep and may pass through a thin wall.
// A swept query per substep is what would lift it.
#include <base/assert.h>
#include <base/report.h>
#include <ecs/component.h>
#include <math/double3.h>
#include <physics/body_system.h>
#include <physics/collider_component.h>
#include <physics/overlap.h>
#include <physics/shape.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <math.h>
#include <stddef.h>

#define MOST_SUBSTEPS 64
#define PUSHES 4
#define CONTACTS 16
// How far below a body a floor still counts as under it, in metres.
#define FLOOR_GAP 1e-3f
// How much further along a step up must land to be kept, in metres.
#define PROGRESS 1e-4f
// The ledge probe's radius, and how far inside the edge it looks.
#define PROBE_RADIUS 0.01f
#define PROBE_INSIDE 0.02f
// A wall normal flatter than this is pushed along itself, not flattened: a
// ceiling, or a floor under a slope limit too small to hold it.
#define LEAST_FLAT 0.5f

struct mover {
	const voe_ecs_world *world;
	voe_ecs_entity self;
	float floor_cos;
	float slope_sin;
	float step_height;
};

struct pose {
	voe_physics_shape shape;
	voe_math_float3 velocity;
	bool on_floor;
	bool met_wall;
};

static const voe_math_float3 up = { 0.0f, 1.0f, 0.0f };

static void shift(struct pose *pose, voe_math_float3 by)
{
	pose->shape.centre = voe_math_double3_add(
		pose->shape.centre, voe_math_double3_from_float3(by));
}

// The deepest contact that blocks, ignoring the body itself and triggers.
static bool deepest(const struct mover *m, voe_physics_shape shape,
		    voe_physics_contact *out)
{
	voe_physics_contact contacts[CONTACTS];
	uint32_t n = voe_physics_overlap(m->world, shape, m->self, contacts,
					 CONTACTS);
	bool found = false;

	VOE_BASE_DEBUG_ASSERT(n <= CONTACTS, "overlap wrote past its capacity");
	for (uint32_t i = 0; i < n; i++) {
		if (contacts[i].trigger)
			continue;
		if (!found || contacts[i].depth > out->depth)
			*out = contacts[i];
		found = true;
	}
	return found;
}

// True when a small sphere just inside and above the contact point touches
// nothing: the body is on the lip of a top no steeper than the slope limit.
static bool on_ledge(const struct mover *m, voe_physics_shape shape,
		     voe_physics_contact contact, voe_math_float3 flat)
{
	float along = shape.kind == VOE_PHYSICS_COLLIDER_CAPSULE ?
			      shape.half.y - shape.half.x :
			      0.0f;
	float rise = (PROBE_RADIUS + PROBE_INSIDE * m->slope_sin) /
		     fmaxf(m->floor_cos, 0.1f);
	voe_math_float3 at = voe_math_float3_scale(
		contact.normal, -(shape.half.x - contact.depth));
	voe_physics_contact ignored;

	VOE_BASE_DEBUG_ASSERT(shape.kind != VOE_PHYSICS_COLLIDER_BOX,
			      "a box body has no ledge probe");
	at.y += rise - along;
	at = voe_math_float3_sub(at, voe_math_float3_scale(flat, PROBE_INSIDE));
	shape.kind = VOE_PHYSICS_COLLIDER_SPHERE;
	shape.half = (voe_math_float3){ PROBE_RADIUS, 0.0f, 0.0f };
	shape.centre = voe_math_double3_add(shape.centre,
					    voe_math_double3_from_float3(at));
	return !deepest(m, shape, &ignored);
}

// Up to PUSHES times, out of the deepest contact: a floor straight up, a wall
// along its flattened normal, dropping the velocity into either.
static void push_out(const struct mover *m, struct pose *pose, bool ledges)
{
	for (uint32_t i = 0; i < PUSHES; i++) {
		voe_physics_contact c;
		voe_math_float3 flat;
		float h;
		float into;

		if (!deepest(m, pose->shape, &c))
			return;
		flat = (voe_math_float3){ c.normal.x, 0.0f, c.normal.z };
		h = voe_math_float3_length(flat);
		if (c.normal.y >= m->floor_cos ||
		    (ledges && c.normal.y > 0.0f && h > 0.0f &&
		     on_ledge(m, pose->shape, c,
			      voe_math_float3_scale(flat, 1.0f / h)))) {
			shift(pose, voe_math_float3_scale(up,
							  c.depth / c.normal.y));
			pose->on_floor = true;
			pose->velocity.y = fmaxf(pose->velocity.y, 0.0f);
			continue;
		}
		if (h < LEAST_FLAT) {
			flat = c.normal;
			h = 1.0f;
		} else {
			flat = voe_math_float3_scale(flat, 1.0f / h);
		}
		shift(pose, voe_math_float3_scale(flat, c.depth / h));
		into = voe_math_float3_dot(pose->velocity, flat);
		if (into < 0.0f) {
			pose->velocity = voe_math_float3_sub(
				pose->velocity,
				voe_math_float3_scale(flat, into));
			pose->met_wall = true;
		}
	}
}

// A floor within FLOOR_GAP below the body.
static bool grounded(const struct mover *m, voe_physics_shape shape)
{
	voe_physics_contact c;

	shape.centre.y -= FLOOR_GAP;
	return deepest(m, shape, &c) && c.normal.y >= m->floor_cos;
}

// Down by `distance` in pieces no longer than a quarter radius, stopping on
// the first floor. False, with the pose moved, when none was found.
static bool settle(const struct mover *m, struct pose *pose, float distance,
		   bool ledges)
{
	float piece = pose->shape.half.x / 4.0f;
	uint32_t pieces = (uint32_t)ceilf(distance / piece);

	VOE_BASE_DEBUG_ASSERT(piece > 0.0f, "settling a body with no radius");
	if (pieces > MOST_SUBSTEPS)
		pieces = MOST_SUBSTEPS;
	if (pieces == 0)
		pieces = 1;
	for (uint32_t i = 0; i < pieces; i++) {
		pose->on_floor = false;
		shift(pose, voe_math_float3_scale(up, -distance / (float)pieces));
		push_out(m, pose, ledges);
		if (pose->on_floor)
			return true;
	}
	return false;
}

static float progress(struct pose from, struct pose to, voe_math_float3 way)
{
	voe_math_float3 moved = voe_math_double3_to_float3(
		voe_math_double3_sub(to.shape.centre, from.shape.centre));

	moved.y = 0.0f;
	return voe_math_float3_dot(moved, way);
}

// From `from`, raised by the step height, moved along the ground, settled
// onto a floor. `first` is kept unless that lands further along.
static struct pose step_up(const struct mover *m, struct pose from,
			   voe_math_float3 motion, struct pose first)
{
	voe_math_float3 ahead = { motion.x, 0.0f, motion.z };
	float length = voe_math_float3_length(ahead);
	voe_math_float3 way;
	struct pose trial = from;

	if (!from.on_floor || !first.met_wall || m->step_height <= 0.0f ||
	    length <= 0.0f)
		return first;
	way = voe_math_float3_scale(ahead, 1.0f / length);
	shift(&trial, voe_math_float3_scale(up, m->step_height));
	push_out(m, &trial, false);
	shift(&trial, ahead);
	push_out(m, &trial, false);
	if (!settle(m, &trial, m->step_height + FLOOR_GAP, true))
		return first;
	if (trial.shape.centre.y - from.shape.centre.y >
	    (double)(m->step_height + FLOOR_GAP))
		return first;
	if (progress(from, trial, way) <= progress(from, first, way) + PROGRESS)
		return first;
	return trial;
}

static struct pose substep(const struct mover *m, struct pose from,
			   float seconds)
{
	voe_math_float3 motion = voe_math_float3_scale(from.velocity, seconds);
	struct pose next = from;

	next.on_floor = false;
	next.met_wall = false;
	shift(&next, motion);
	push_out(m, &next, false);
	next.on_floor = next.on_floor || grounded(m, next.shape);
	return step_up(m, from, motion, next);
}

// The whole of one body's move from `start`; its centre and velocity come out.
static struct pose move_one(const struct mover *m, struct pose start,
			    float seconds)
{
	float reach = voe_math_float3_length(start.velocity) * seconds;
	uint32_t count = (uint32_t)ceilf(reach / (start.shape.half.x / 4.0f));
	struct pose pose = start;
	bool floored = start.on_floor;

	VOE_BASE_DEBUG_ASSERT(start.shape.half.x > 0.0f, "a body with no radius");
	if (count > MOST_SUBSTEPS)
		count = MOST_SUBSTEPS;
	if (count == 0)
		count = 1;
	for (uint32_t i = 0; i < count; i++) {
		pose = substep(m, pose, seconds / (float)count);
		floored = floored || pose.on_floor;
	}
	if (floored && !pose.on_floor && start.velocity.y <= 0.0f) {
		struct pose probe = pose;

		if (settle(m, &probe, m->step_height + FLOOR_GAP, false))
			pose = probe;
	}
	return pose;
}

// Moves one body and writes its row; false when its transform intent did not
// fit in the queue.
static bool move_body(voe_ecs_world *world, voe_ecs_type type,
		      voe_ecs_entity entity, voe_physics_body row,
		      float seconds)
{
	const voe_scene_transform *transform = voe_scene_transform_get(world, entity);
	voe_physics_shape shape;
	struct mover m = { .world = world,
			   .self = entity,
			   .floor_cos = cosf(row.slope_limit),
			   .slope_sin = sinf(row.slope_limit),
			   .step_height = row.step_height };
	struct pose end;
	voe_math_double3 moved;
	voe_scene_transform_intent intent;

	if (transform == NULL || !voe_physics_shape_of(world, entity, &shape) ||
	    shape.kind == VOE_PHYSICS_COLLIDER_BOX)
		return true;
	end = move_one(&m,
		       (struct pose){ .shape = shape,
				      .velocity = row.velocity,
				      .on_floor = row.on_floor },
		       seconds);
	moved = voe_math_double3_sub(end.shape.centre, shape.centre);
	intent = (voe_scene_transform_intent){ .entity = entity,
					       .transform = *transform };
	intent.transform.position =
		voe_math_double3_add(transform->position, moved);
	if (!voe_scene_transform_submit(world, intent))
		return false;
	row.velocity = voe_math_float3_scale(voe_math_double3_to_float3(moved),
					     1.0f / seconds);
	row.on_floor = end.on_floor;
	return voe_ecs_component_set(world, type, entity, &row);
}

void voe_physics_body_system_move(voe_ecs_world *world, float seconds)
{
	voe_ecs_type type;
	uint32_t count;
	uint32_t dropped = 0;

	VOE_BASE_ASSERT(world != NULL, "moving the bodies of no world");
	VOE_BASE_ASSERT(isfinite(seconds) && seconds > 0.0f,
			"moving bodies by no time");

	type = voe_ecs_component_type(world, &voe_physics_body_key);
	count = voe_physics_body_count(world);
	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_entity entity = voe_physics_body_entities(world)[i];
		voe_physics_body row = voe_physics_body_rows(world)[i];

		if (!move_body(world, type, entity, row, seconds))
			dropped++;
	}
	if (dropped > 0)
		VOE_BASE_ERROR("physics",
			       "voe_physics_body: %u moves dropped, the transform queue is full",
			       (unsigned)dropped);
}
