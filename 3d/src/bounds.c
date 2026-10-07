// The walk that boxes an entity tree's vertices and the distance that frames
// its sphere — see the header for what counts and why a sphere.
//
// One box about one double origin is grown by every counted geometry, shapes
// first, then models, a landscape by its box's eight corners; the origin is
// fixed by the first one counted.
#include <3d/bounds.h>
#include <3d/landscape.h>
#include <3d/model_component.h>
#include <3d/shape_component.h>

#include <base/assert.h>

#include <ecs/component.h>

#include <math/float4x4.h>

#include <scene/parent_component.h>
#include <scene/transform_component.h>

#include <math.h>
#include <stddef.h>

// The box grown so far, in float about `origin`; `counted` false until the
// first geometry fixes the origin.
struct box {
	bool counted;
	voe_math_double3 origin;
	voe_math_float3 min;
	voe_math_float3 max;
};

// Whether the world registered the store under `key`. A walk of the types
// rather than voe_ecs_component_type, which asserts on a key nothing
// registered, as pick.c's has_store does.
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

// Whether `entity` is `root` or under it and placed somewhere.
static bool counts(const voe_ecs_world *world, voe_ecs_entity entity,
		   voe_ecs_entity root)
{
	return voe_scene_parent_within(world, entity, root) &&
	       voe_scene_transform_get(world, entity) != NULL;
}

// `entity`'s world matrix about the box's origin, fixing that origin at the
// entity's place when nothing has been counted yet.
static voe_math_float4x4 placed_about(struct box *box,
				      const voe_ecs_world *world,
				      voe_ecs_entity entity)
{
	voe_scene_transform placed = voe_scene_transform_world(world, entity);

	if (!box->counted)
		box->origin = placed.position;
	return voe_scene_transform_matrix(placed, box->origin);
}

// Grows `box` by one own-space `point` carried by `matrix`.
static void take(struct box *box, voe_math_float4x4 matrix,
		 voe_math_float3 point)
{
	point = voe_math_float4x4_transform_point(matrix, point);
	if (!box->counted) {
		box->min = point;
		box->max = point;
		box->counted = true;
		return;
	}
	box->min = voe_math_float3_min(box->min, point);
	box->max = voe_math_float3_max(box->max, point);
}

// Grows `box` by every vertex of `geometry` placed at `entity`'s world place.
static void grow(struct box *box, const voe_ecs_world *world,
		 voe_ecs_entity entity, const voe_3d_shape_geometry *geometry)
{
	voe_math_float4x4 matrix;

	VOE_BASE_ASSERT(box != NULL && geometry != NULL,
			"a box grown from nothing");

	if (geometry->vertex_count == 0)
		return;
	matrix = placed_about(box, world, entity);
	for (uint32_t i = 0; i < geometry->vertex_count; i++)
		take(box, matrix, geometry->vertices[i].position);
}

// Grows `box` by the eight corners of `landscape`'s own box (0379 point 2)
// placed at `entity`'s world place, as a model's vertices are.
static void grow_landscape(struct box *box, const voe_ecs_world *world,
			   voe_ecs_entity entity,
			   const voe_assets_landscape *landscape)
{
	voe_math_float4x4 matrix = placed_about(box, world, entity);
	voe_math_float3 low;
	voe_math_float3 high;

	voe_3d_landscape_box(landscape, &low, &high);
	for (uint32_t corner = 0; corner < 8; corner++)
		take(box, matrix,
		     (voe_math_float3){ corner & 1 ? high.x : low.x,
					corner & 2 ? high.y : low.y,
					corner & 4 ? high.z : low.z });
}

bool voe_3d_bounds(const voe_ecs_world *world,
		   const voe_3d_shape_geometries *geometries,
		   const voe_3d_models *models, voe_ecs_entity root,
		   voe_math_double3 *centre, float *radius)
{
	struct box box = { 0 };

	VOE_BASE_ASSERT(world != NULL && geometries != NULL,
			"bounds of nothing");
	VOE_BASE_ASSERT(centre != NULL && radius != NULL,
			"bounds answered into nothing");

	if (has_store(world, &voe_3d_shape_key)) {
		uint32_t count = voe_3d_shape_count(world);
		const voe_3d_shape *rows = voe_3d_shape_rows(world);
		const voe_ecs_entity *entities = voe_3d_shape_entities(world);

		for (uint32_t i = 0; i < count; i++) {
			const voe_3d_shape_geometry *geometry =
				voe_3d_shape_geometry_of(geometries,
							 rows[i].kind);

			if (geometry != NULL && counts(world, entities[i], root))
				grow(&box, world, entities[i], geometry);
		}
	}

	if (models != NULL && has_store(world, &voe_3d_model_key)) {
		uint32_t count = voe_3d_model_count(world);
		const voe_3d_model *worn = voe_3d_model_rows(world);
		const voe_ecs_entity *owners = voe_3d_model_entities(world);

		for (uint32_t i = 0; i < count; i++) {
			const voe_3d_model_entry *entry =
				voe_3d_models_find(models, worn[i].path);

			if (entry == NULL || !entry->loaded ||
			    !counts(world, owners[i], root))
				continue;
			if (entry->landscape != NULL)
				grow_landscape(&box, world, owners[i],
					       entry->landscape);
			else
				grow(&box, world, owners[i], &entry->shape);
		}
	}

	if (!box.counted)
		return false;
	*centre = voe_math_double3_add(
		box.origin,
		voe_math_double3_from_float3(voe_math_float3_scale(
			voe_math_float3_add(box.min, box.max), 0.5f)));
	*radius = 0.5f * voe_math_float3_length(
				 voe_math_float3_sub(box.max, box.min));
	return true;
}

float voe_3d_bounds_distance(float radius, float fov_y, float aspect,
			     float fill)
{
	float vertical = fov_y * 0.5f;
	float horizontal = atanf(aspect * tanf(vertical));
	float half = fminf(vertical, horizontal);

	return radius / sinf(atanf(fill * tanf(half)));
}
