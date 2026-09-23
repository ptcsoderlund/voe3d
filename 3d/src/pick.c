// The pick ray and the walk that answers it — see the header for why this
// question is answered here and against which table.
#include <3d/pick.h>
#include <3d/shape_component.h>

#include <base/assert.h>

#include <math/float4.h>
#include <math/float4x4.h>

#include <scene/transform_component.h>

#include <stddef.h>

// Nearer than this along the ray is the surface the origin is already sitting
// on, not a hit: a tenth of a millimetre, the same tolerance the geometry is
// welded with (3d/shape_geometry.h).
#define NEAR_ENOUGH_TO_BE_HERE 1e-4f

// A triangle edge the ray runs along gives a determinant of nothing, and the
// division below would be by nothing. Below this the ray misses rather than
// divides.
#define PARALLEL 1e-9f

// The clip-space point of a pixel's centre at depth `z`, taken back into world
// space by `inverse` and divided by its own w.
static voe_math_float3 unproject(voe_math_float4x4 inverse, float x, float y,
				 float z)
{
	voe_math_float4 clip = { x, y, z, 1.0f };
	voe_math_float4 world = voe_math_float4x4_mul_float4(inverse, clip);

	return (voe_math_float3){ world.x / world.w, world.y / world.w,
				  world.z / world.w };
}

// Möller–Trumbore, once. A point of the triangle is a + u * (b - a) +
// v * (c - a), and a point of the ray is origin + t * direction, so
//
//     origin - a = u * (b - a) + v * (c - a) - t * direction
//
// is three equations in u, v and t. Cramer's rule on that 3x3 system is the
// arithmetic below: the determinant is (direction x ac) . ab, and each unknown
// is the same triple product with one column swapped for (origin - a). A hit is
// u >= 0, v >= 0, u + v <= 1 and t in front of the origin.
//
// Both faces count: a ray leaving a shape through its far side is still inside
// that shape, and refusing a back face would let a click through a box a person
// is standing in.
static bool ray_hits_triangle(voe_3d_ray ray, voe_math_float3 a,
			      voe_math_float3 b, voe_math_float3 c, float *t)
{
	voe_math_float3 ab = voe_math_float3_sub(b, a);
	voe_math_float3 ac = voe_math_float3_sub(c, a);
	voe_math_float3 p = voe_math_float3_cross(ray.direction, ac);
	float determinant = voe_math_float3_dot(ab, p);
	voe_math_float3 to_origin;
	voe_math_float3 q;
	float inverse;
	float u;
	float v;
	float hit;

	if (determinant > -PARALLEL && determinant < PARALLEL)
		return false;

	inverse = 1.0f / determinant;
	to_origin = voe_math_float3_sub(ray.origin, a);
	u = voe_math_float3_dot(to_origin, p) * inverse;
	if (u < 0.0f || u > 1.0f)
		return false;

	q = voe_math_float3_cross(to_origin, ab);
	v = voe_math_float3_dot(ray.direction, q) * inverse;
	if (v < 0.0f || u + v > 1.0f)
		return false;

	hit = voe_math_float3_dot(ac, q) * inverse;
	if (hit <= NEAR_ENOUGH_TO_BE_HERE)
		return false;

	*t = hit;
	return true;
}

voe_3d_ray voe_3d_pick_ray(voe_render_view view, voe_platform_size size,
			   voe_math_float2 point)
{
	float width = (float)size.width;
	float height = (float)size.height;
	voe_math_float4x4 inverse;
	voe_math_float3 near_point;
	voe_math_float3 far_point;
	float x;
	float y;

	VOE_BASE_ASSERT(size.width > 0 && size.height > 0,
			"a pick ray through a picture with no area");

	// The two matrices the pass was opened with, inverted as one product:
	// clip to world in a single multiply per point.
	inverse = voe_math_float4x4_inverse(
		voe_math_float4x4_mul(view.projection, view.view));

	x = 2.0f * (point.x + 0.5f) / width - 1.0f;
	// THE ONE LINE THAT FLIPS Y, and the only one that may: the picture's y
	// runs down from its top-left corner and clip space's runs up.
	y = 1.0f - 2.0f * (point.y + 0.5f) / height;

	// Depth runs backwards (3d/projection.h): the near plane is at 1 and the
	// far plane at 0.
	near_point = unproject(inverse, x, y, 1.0f);
	far_point = unproject(inverse, x, y, 0.0f);

	return (voe_3d_ray){ .origin = near_point,
			     .direction = voe_math_float3_normalize(
				     voe_math_float3_sub(far_point,
							 near_point)) };
}

voe_ecs_entity voe_3d_pick(const voe_ecs_world *world,
			   const voe_3d_shape_geometries *geometries,
			   voe_3d_ray ray, float *distance)
{
	uint32_t count = voe_3d_shape_count(world);
	const voe_3d_shape *rows = voe_3d_shape_rows(world);
	const voe_ecs_entity *entities = voe_3d_shape_entities(world);
	voe_ecs_entity hit = { 0 };
	float nearest = 0.0f;

	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_transform *transform =
			voe_scene_transform_get(world, entities[i]);
		const voe_3d_shape_geometry *geometry =
			voe_3d_shape_geometry_of(geometries, rows[i].kind);
		voe_math_float4x4 matrix;
		voe_3d_ray local;

		if (transform == NULL || geometry == NULL)
			continue;

		matrix = voe_scene_transform_matrix(*transform);
		// Scaled away to nothing: drawn as nothing, and its matrix
		// cannot be inverted (math/float4x4.h asserts on a singular
		// one).
		if (voe_math_float4x4_determinant(matrix) == 0.0f)
			continue;

		matrix = voe_math_float4x4_inverse(matrix);
		// The direction is carried over and not normalised again, so
		// `t` below is the same number in both spaces — see the header.
		local.origin =
			voe_math_float4x4_transform_point(matrix, ray.origin);
		local.direction =
			voe_math_float4x4_transform_dir(matrix, ray.direction);

		for (uint32_t j = 0; j + 2 < geometry->index_count; j += 3) {
			float t;

			if (!ray_hits_triangle(
				    local,
				    geometry->vertices[geometry->indices[j]]
					    .position,
				    geometry->vertices[geometry->indices[j + 1]]
					    .position,
				    geometry->vertices[geometry->indices[j + 2]]
					    .position,
				    &t))
				continue;
			if (hit.generation != 0 && t >= nearest)
				continue;
			nearest = t;
			hit = entities[i];
		}
	}

	if (hit.generation != 0 && distance != NULL)
		*distance = nearest;
	return hit;
}
