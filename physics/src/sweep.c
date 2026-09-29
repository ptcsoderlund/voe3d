// The gather and the sweep: the segment `from` to `from + motion` in float
// about `from`, tested against each gathered obstacle in turn.
//
// A sphere and a capsule are one test: a capsule of segment half-length h
// about its local Y, where a sphere's h is 0. In the capsule's own frame the
// sweep is a ray against the infinite cylinder of the summed radius, kept
// only where it lands within the segment, then against the two cap spheres;
// the least t of those is where the ray enters. A sweep starting within the
// summed radius of the segment is an overlap and hits at t 0.
#include <base/assert.h>
#include <math/double3.h>
#include <math/quat.h>
#include <physics/collider_component.h>
#include <physics/sweep.h>

#include <math.h>

// A candidate hit about `from`: the fraction and the unit normal.
struct touch {
	float t;
	voe_math_float3 normal;
};

static voe_math_float3 rotate(voe_math_quat q, voe_math_float3 v)
{
	const voe_math_float3 u = { q.x, q.y, q.z };
	const voe_math_float3 t =
		voe_math_float3_scale(voe_math_float3_cross(u, v), 2.0f);

	return voe_math_float3_add(
		voe_math_float3_add(v, voe_math_float3_scale(t, q.w)),
		voe_math_float3_cross(u, t));
}

static float reach_of(voe_physics_shape shape)
{
	if (shape.kind == VOE_PHYSICS_COLLIDER_SPHERE)
		return shape.half.x;
	if (shape.kind == VOE_PHYSICS_COLLIDER_CAPSULE)
		return shape.half.y;
	return voe_math_float3_length(shape.half);
}

// The distance from `centre` to the segment 0..motion.
static float segment_distance(voe_math_float3 motion, voe_math_float3 centre)
{
	const float length = voe_math_float3_dot(motion, motion);
	float t = 0.0f;

	if (length > 1e-12f)
		t = fminf(fmaxf(voe_math_float3_dot(centre, motion) / length,
				0.0f),
			  1.0f);
	return voe_math_float3_length(
		voe_math_float3_sub(centre, voe_math_float3_scale(motion, t)));
}

// The ray o + v t, t in 0..1, entering the sphere of `reach` about `centre`,
// kept in *best when earlier. The ray starts outside or on it.
static void ray_sphere(voe_math_float3 o, voe_math_float3 v,
		       voe_math_float3 centre, float reach, struct touch *best)
{
	const voe_math_float3 d = voe_math_float3_sub(o, centre);
	const float a = voe_math_float3_dot(v, v);
	const float b = voe_math_float3_dot(v, d);
	const float c = voe_math_float3_dot(d, d) - reach * reach;
	const float disc = b * b - a * c;
	float t;

	if (a < 1e-12f || disc < 0.0f)
		return;
	t = (-b - sqrtf(disc)) / a;
	if (t < 0.0f || t > 1.0f || t >= best->t)
		return;
	best->t = t;
	best->normal = voe_math_float3_normalize(voe_math_float3_sub(
		voe_math_float3_add(o, voe_math_float3_scale(v, t)), centre));
}

// The ray o + v t against the side of the cylinder of `reach` about local Y,
// kept in *best when it lands within |y| <= h and is earlier.
static void ray_cylinder(voe_math_float3 o, voe_math_float3 v, float h,
			 float reach, struct touch *best)
{
	const float a = v.x * v.x + v.z * v.z;
	const float b = o.x * v.x + o.z * v.z;
	const float c = o.x * o.x + o.z * o.z - reach * reach;
	const float disc = b * b - a * c;
	float t;
	float y;

	if (a < 1e-12f || disc < 0.0f)
		return;
	t = (-b - sqrtf(disc)) / a;
	y = o.y + v.y * t;
	if (t < 0.0f || t > 1.0f || t >= best->t || fabsf(y) > h)
		return;
	best->t = t;
	best->normal = voe_math_float3_normalize(
		(voe_math_float3){ o.x + v.x * t, 0.0f, o.z + v.z * t });
}

// The sweep against a sphere or capsule whose centre is at `offset` about
// `from`: true and *out filled when it touches within the motion.
static bool touch_round(voe_physics_shape shape, voe_math_float3 offset,
			voe_math_float3 motion, float radius,
			struct touch *out)
{
	const voe_math_quat back = { -shape.rotation.x, -shape.rotation.y,
				     -shape.rotation.z, shape.rotation.w };
	const voe_math_float3 o = rotate(back, voe_math_float3_neg(offset));
	const voe_math_float3 v = rotate(back, motion);
	const float h = shape.kind == VOE_PHYSICS_COLLIDER_CAPSULE ?
				shape.half.y - shape.half.x :
				0.0f;
	const float reach = radius + shape.half.x;
	const voe_math_float3 nearest = { 0.0f, fminf(fmaxf(o.y, -h), h), 0.0f };
	struct touch best = { .t = INFINITY };

	if (voe_math_float3_length(voe_math_float3_sub(o, nearest)) < reach) {
		const float length = voe_math_float3_length(motion);

		out->t = 0.0f;
		out->normal = length > 1e-12f ?
				      voe_math_float3_scale(motion, -1.0f / length) :
				      (voe_math_float3){ 0.0f, 1.0f, 0.0f };
		return true;
	}
	if (h > 0.0f)
		ray_cylinder(o, v, h, reach, &best);
	ray_sphere(o, v, (voe_math_float3){ 0.0f, h, 0.0f }, reach, &best);
	ray_sphere(o, v, (voe_math_float3){ 0.0f, -h, 0.0f }, reach, &best);
	if (!(best.t <= 1.0f))
		return false;
	out->t = best.t;
	out->normal = rotate(shape.rotation, best.normal);
	return true;
}

uint32_t voe_physics_obstacles_gather(const voe_ecs_world *world,
				      voe_physics_obstacle *out,
				      uint32_t capacity)
{
	uint32_t count;
	const voe_physics_collider *rows;
	const voe_ecs_entity *entities;
	uint32_t written = 0;

	VOE_BASE_ASSERT(world != NULL && (out != NULL || capacity == 0),
			"a gather from no world, or into nowhere");
	count = voe_physics_collider_count(world);
	rows = voe_physics_collider_rows(world);
	entities = voe_physics_collider_entities(world);

	for (uint32_t i = 0; i < count && written < capacity; i++) {
		voe_physics_shape shape;

		if (rows[i].trigger ||
		    !voe_physics_shape_of(world, entities[i], &shape))
			continue;
		out[written++] = (voe_physics_obstacle){
			.entity = entities[i],
			.shape = shape,
			.reach = reach_of(shape) };
	}
	VOE_BASE_DEBUG_ASSERT(written <= capacity, "wrote past capacity");
	return written;
}

bool voe_physics_sweep(const voe_physics_obstacle *obstacles, uint32_t count,
		       voe_math_double3 from, voe_math_float3 motion,
		       float radius, voe_ecs_entity ignore, voe_physics_hit *out)
{
	struct touch best = { .t = INFINITY };
	uint32_t found = count;

	VOE_BASE_ASSERT((obstacles != NULL || count == 0) && out != NULL,
			"a sweep through nothing, or into nowhere");
	VOE_BASE_ASSERT(radius >= 0.0f && isfinite(radius),
			"a sweep's radius is finite and not below 0");
	VOE_BASE_ASSERT(isfinite(motion.x) && isfinite(motion.y) &&
				isfinite(motion.z),
			"a sweep's motion is finite");

	for (uint32_t i = 0; i < count; i++) {
		const voe_physics_obstacle *it = &obstacles[i];
		const voe_math_float3 offset = voe_math_double3_to_float3(
			voe_math_double3_sub(it->shape.centre, from));
		struct touch touch;

		if (it->entity.index == ignore.index &&
		    it->entity.generation == ignore.generation)
			continue;
		if (it->shape.kind == VOE_PHYSICS_COLLIDER_BOX ||
		    segment_distance(motion, offset) > it->reach + radius)
			continue;
		if (touch_round(it->shape, offset, motion, radius, &touch) &&
		    touch.t < best.t) {
			best = touch;
			found = i;
		}
	}
	if (found == count)
		return false;
	*out = (voe_physics_hit){
		.entity = obstacles[found].entity,
		.t = best.t,
		.point = voe_math_double3_add(
			from, voe_math_double3_from_float3(voe_math_float3_sub(
				      voe_math_float3_scale(motion, best.t),
				      voe_math_float3_scale(best.normal,
							    radius)))),
		.normal = best.normal };
	return true;
}
