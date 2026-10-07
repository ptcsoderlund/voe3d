// The pick ray and the walk that answers it — see the header for why this
// question is answered here and against which table.
//
// The view's two matrices are inverted once per pick. The walk carries the ray
// into each shape's, model's or water's own space, from its world place, and
// tests its triangles, for a landscape its heights (3d/landscape.h), or for a
// water its plane; the triangle test is written
// out once, its derivation in a comment above it. The markers — cameras' boxes,
// suns', point lights' and place markers' cubes — are tested as one group, and
// its nearest hit wins over any mesh hit; the meshes are walked only when no
// marker is met.
#include <3d/camera_marker.h>
#include <3d/landscape.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/pick.h>
#include <3d/place_marker.h>
#include <3d/point_light_marker.h>
#include <3d/shape_component.h>
#include <3d/sun_marker.h>
#include <3d/water_component.h>

#include <base/assert.h>

#include <math/float4.h>
#include <math/float4x4.h>

#include <ecs/component.h>

#include <scene/camera_component.h>
#include <scene/light_component.h>
#include <scene/point_light_component.h>
#include <scene/transform_component.h>

#include <math.h>
#include <stddef.h>

// Nearer than this along the ray is the surface the origin is already sitting
// on, not a hit: a tenth of a millimetre, the same tolerance the geometry is
// welded with (3d/shape_geometry.h).
#define NEAR_ENOUGH_TO_BE_HERE 1e-4f

// A triangle edge the ray runs along gives a determinant of nothing, and the
// division below would be by nothing. Below this the ray misses rather than
// divides.
#define PARALLEL 1e-9f

// A ray in one entity's own space, all float: the world ray's origin has been
// subtracted in double before anything was narrowed.
struct local_ray {
	voe_math_float3 origin;
	voe_math_float3 direction;
};

// The clip-space point of a pixel's centre at depth `z`, taken back into
// eye-relative space by `inverse` and divided by its own w.
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
static bool ray_hits_triangle(struct local_ray ray, voe_math_float3 a,
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

// Whether the world registered the store under `key`. A walk of the types
// rather than voe_ecs_component_type, which asserts on a key nothing
// registered, as draw_system.c's shape_type walks them; once per pick.
static bool has_store(const voe_ecs_world *world, const struct voe_ecs_key *key)
{
	uint32_t count = voe_ecs_component_type_count(world);

	for (uint32_t i = 0; i < count; i++)
		if (voe_ecs_component_key(world,
					  voe_ecs_component_type_at(world, i)) ==
		    key)
			return true;
	return false;
}

voe_3d_ray voe_3d_pick_ray(voe_render_view view, voe_math_double3 eye,
			   voe_platform_size size, voe_math_float2 point)
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
	// clip to eye-relative space in a single multiply per point.
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

	return (voe_3d_ray){ .origin = voe_math_double3_add(
				     eye, voe_math_double3_from_float3(
						  near_point)),
			     .direction = voe_math_float3_normalize(
				     voe_math_float3_sub(far_point,
							 near_point)) };
}

// `ray` carried into the own space of an entity placed by `transform`, false
// when it is scaled away to nothing. Shapes, models and waters alike.
static bool carried_into(voe_scene_transform transform, voe_3d_ray ray,
			 struct local_ray *local)
{
	voe_math_float4x4 matrix;

	VOE_BASE_ASSERT(local != NULL, "a ray carried into nothing");

	// About the ray's origin, so the ray starts at nought and only the
	// entity's small offset from it is narrowed (ADR-0250).
	matrix = voe_scene_transform_matrix(transform, ray.origin);
	// Scaled away to nothing: drawn as nothing, and its matrix cannot be
	// inverted (math/float4x4.h asserts on a singular one).
	if (voe_math_float4x4_determinant(matrix) == 0.0f)
		return false;

	matrix = voe_math_float4x4_inverse(matrix);
	// The direction is carried over and not normalised again, so a `t` is
	// the same number in both spaces — see the header.
	local->origin = voe_math_float4x4_transform_point(
		matrix, (voe_math_float3){ 0.0f, 0.0f, 0.0f });
	local->direction =
		voe_math_float4x4_transform_dir(matrix, ray.direction);
	return true;
}

// Where `ray` crosses a water's plane, its own y = 0 within half its width
// along x and half its length along z, from either face (0365 point 3).
static bool water_hit(voe_scene_transform transform, voe_3d_water water,
		      voe_3d_ray ray, float *t)
{
	struct local_ray local;
	float hit;
	float x;
	float z;

	VOE_BASE_ASSERT(t != NULL, "a hit measured into nothing");

	if (!carried_into(transform, ray, &local))
		return false;
	// A ray along the plane never crosses it, and would divide by nothing.
	if (local.direction.y > -PARALLEL && local.direction.y < PARALLEL)
		return false;
	hit = -local.origin.y / local.direction.y;
	if (hit <= NEAR_ENOUGH_TO_BE_HERE)
		return false;
	x = local.origin.x + hit * local.direction.x;
	z = local.origin.z + hit * local.direction.z;
	if (fabsf(x) > water.width * 0.5f || fabsf(z) > water.length * 0.5f)
		return false;
	*t = hit;
	return true;
}

// The nearest hit along `ray` of `geometry` placed by `transform`, in metres,
// false when it is missed or scaled away to nothing. Shapes and models alike.
static bool geometry_hit(voe_scene_transform transform,
			 const voe_3d_shape_geometry *geometry, voe_3d_ray ray,
			 float *nearest)
{
	struct local_ray local;
	bool hit = false;

	VOE_BASE_ASSERT(geometry != NULL, "a ray against no geometry");
	VOE_BASE_ASSERT(nearest != NULL, "a hit measured into nothing");

	if (!carried_into(transform, ray, &local))
		return false;

	for (uint32_t j = 0; j + 2 < geometry->index_count; j += 3) {
		float t;

		if (!ray_hits_triangle(
			    local,
			    geometry->vertices[geometry->indices[j]].position,
			    geometry->vertices[geometry->indices[j + 1]].position,
			    geometry->vertices[geometry->indices[j + 2]].position,
			    &t))
			continue;
		if (hit && t >= *nearest)
			continue;
		*nearest = t;
		hit = true;
	}
	return hit;
}

// Where `ray` first goes below `landscape`'s heights placed by `transform`, in
// the grid's own space and as a parameter along the ray that is the same in
// both spaces, as geometry_hit's is. False when missed or scaled away.
static bool landscape_hit(voe_scene_transform transform,
			  const voe_assets_landscape *landscape, voe_3d_ray ray,
			  voe_3d_landscape_hit *hit)
{
	struct local_ray local;
	float t;

	VOE_BASE_ASSERT(landscape != NULL && hit != NULL,
			"a landscape hit measured into nothing");

	if (!carried_into(transform, ray, &local) ||
	    !voe_3d_landscape_ray(landscape, local.origin, local.direction, &t))
		return false;
	hit->x = local.origin.x + t * local.direction.x;
	hit->z = local.origin.z + t * local.direction.z;
	hit->distance = t;
	return true;
}

// The nearest hit of one group so far: zeroed until something is met.
struct nearest_hit {
	voe_ecs_entity entity;
	float distance;
};

// Keeps `entity` at `t` when it is the group's first hit or nearer than it.
static void keep_nearer(struct nearest_hit *best, voe_ecs_entity entity,
			float t)
{
	VOE_BASE_ASSERT(best != NULL, "a hit kept in nothing");
	VOE_BASE_ASSERT(entity.generation != 0, "a hit on no entity");

	if (best->entity.generation != 0 && t >= best->distance)
		return;
	best->entity = entity;
	best->distance = t;
}

// The shapes, models and waters: everything clicked on its own surface.
static struct nearest_hit mesh_hit(const voe_ecs_world *world,
				   const voe_3d_shape_geometries *geometries,
				   const voe_3d_models *models, voe_3d_ray ray)
{
	uint32_t count = voe_3d_shape_count(world);
	const voe_3d_shape *rows = voe_3d_shape_rows(world);
	const voe_ecs_entity *entities = voe_3d_shape_entities(world);
	struct nearest_hit best = { 0 };

	for (uint32_t i = 0; i < count; i++) {
		const voe_3d_shape_geometry *geometry =
			voe_3d_shape_geometry_of(geometries, rows[i].kind);
		float t;

		if (voe_scene_transform_get(world, entities[i]) != NULL &&
		    geometry != NULL &&
		    geometry_hit(voe_scene_transform_world(world, entities[i]),
				 geometry, ray, &t))
			keep_nearer(&best, entities[i], t);
	}

	// A model row is tested on its loaded entry's own triangles, exactly as
	// a shape is on its kind's (ADR-0277 point 3); a landscape's shape is
	// empty, so it is tested on its heights (0379 point 2).
	if (models != NULL && has_store(world, &voe_3d_model_key)) {
		uint32_t wearers = voe_3d_model_count(world);
		const voe_3d_model *worn = voe_3d_model_rows(world);
		const voe_ecs_entity *owners = voe_3d_model_entities(world);

		for (uint32_t i = 0; i < wearers; i++) {
			const voe_3d_model_entry *entry =
				voe_3d_models_find(models, worn[i].path);
			voe_scene_transform placed;
			voe_3d_landscape_hit ground;
			float t;

			if (voe_scene_transform_get(world, owners[i]) == NULL ||
			    entry == NULL || !entry->loaded)
				continue;
			placed = voe_scene_transform_world(world, owners[i]);
			if (entry->landscape != NULL) {
				if (landscape_hit(placed, entry->landscape, ray,
						  &ground))
					keep_nearer(&best, owners[i],
						    ground.distance);
			} else if (geometry_hit(placed, &entry->shape, ray, &t)) {
				keep_nearer(&best, owners[i], t);
			}
		}
	}

	// A water on its plane, never its waves, which only bend the shading
	// (3d/water_component.h).
	if (has_store(world, &voe_3d_water_key)) {
		uint32_t waters = voe_3d_water_count(world);
		const voe_3d_water *bodies = voe_3d_water_rows(world);
		const voe_ecs_entity *owners = voe_3d_water_entities(world);

		for (uint32_t i = 0; i < waters; i++) {
			float t;

			if (voe_scene_transform_get(world, owners[i]) != NULL &&
			    water_hit(voe_scene_transform_world(world, owners[i]),
				      bodies[i], ray, &t))
				keep_nearer(&best, owners[i], t);
		}
	}
	return best;
}

// The cameras, suns, point lights and places: everything clicked on a marker.
static struct nearest_hit marker_hit(const voe_ecs_world *world,
				     voe_3d_ray ray)
{
	struct nearest_hit best = { 0 };

	VOE_BASE_ASSERT(world != NULL, "a pick in no world");

	// The cameras on their box, never their frustum (0223,
	// 3d/camera_marker.h).
	if (has_store(world, &voe_scene_camera_key)) {
		uint32_t cameras = voe_scene_camera_count(world);
		const voe_ecs_entity *owners = voe_scene_camera_entities(world);

		for (uint32_t i = 0; i < cameras; i++) {
			float t;

			if (voe_scene_transform_get(world, owners[i]) != NULL &&
			    voe_3d_camera_marker_hit(
				    voe_scene_transform_world(world, owners[i]),
				    ray, &t))
				keep_nearer(&best, owners[i], t);
		}
	}

	// The suns on their marker's cube, never its lines (0274,
	// 3d/sun_marker.h).
	if (has_store(world, &voe_scene_light_key)) {
		uint32_t lights = voe_scene_light_count(world);
		const voe_ecs_entity *owners = voe_scene_light_entities(world);

		for (uint32_t i = 0; i < lights; i++) {
			float t;

			if (voe_scene_transform_get(world, owners[i]) != NULL &&
			    voe_3d_sun_marker_hit(
				    voe_scene_transform_world(world, owners[i]),
				    ray, &t))
				keep_nearer(&best, owners[i], t);
		}
	}

	// The point lights on their marker's world-axis cube at their world
	// place (0320 point 7, 3d/point_light_marker.h).
	if (has_store(world, &voe_scene_point_light_key)) {
		uint32_t lamps = voe_scene_point_light_count(world);
		const voe_ecs_entity *owners =
			voe_scene_point_light_entities(world);

		for (uint32_t i = 0; i < lamps; i++) {
			float t;

			if (voe_scene_transform_get(world, owners[i]) != NULL &&
			    voe_3d_point_light_marker_hit(
				    voe_scene_transform_world(world, owners[i])
					    .position,
				    ray, &t))
				keep_nearer(&best, owners[i], t);
		}
	}

	// Every other place on its diamond's cube: the transform table's owners
	// that wear it, the one answer the draw asks too (3d/place_marker.h).
	{
		uint32_t places = voe_scene_transform_count(world);
		const voe_ecs_entity *owners =
			voe_scene_transform_entities(world);

		for (uint32_t i = 0; i < places; i++) {
			float t;

			if (voe_3d_place_marker_wanted(world, owners[i]) &&
			    voe_3d_place_marker_hit(
				    voe_scene_transform_world(world, owners[i])
					    .position,
				    ray, &t))
				keep_nearer(&best, owners[i], t);
		}
	}
	return best;
}

voe_ecs_entity voe_3d_pick(const voe_ecs_world *world,
			   const voe_3d_shape_geometries *geometries,
			   const voe_3d_models *models, voe_3d_ray ray,
			   float *distance)
{
	struct nearest_hit marker = marker_hit(world, ray);
	struct nearest_hit answer =
		marker.entity.generation != 0 ?
			marker :
			mesh_hit(world, geometries, models, ray);

	if (answer.entity.generation != 0 && distance != NULL)
		*distance = answer.distance;
	return answer.entity;
}

bool voe_3d_pick_landscape(const voe_ecs_world *world,
			   const voe_3d_models *models, voe_ecs_entity entity,
			   voe_3d_ray ray, voe_3d_landscape_hit *hit)
{
	const voe_3d_model *worn;
	const voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(world != NULL && hit != NULL,
			"a landscape picked into nothing");

	if (models == NULL || !has_store(world, &voe_3d_model_key) ||
	    voe_scene_transform_get(world, entity) == NULL)
		return false;
	worn = voe_3d_model_get(world, entity);
	if (worn == NULL)
		return false;
	entry = voe_3d_models_find(models, worn->path);
	if (entry == NULL || !entry->loaded || entry->landscape == NULL)
		return false;
	return landscape_hit(voe_scene_transform_world(world, entity),
			     entry->landscape, ray, hit);
}
