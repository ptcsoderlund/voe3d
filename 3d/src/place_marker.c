// The place marker's twelve diamond edges in world axes about its position,
// handed to marker_lines.c for their quads, its cube's slab test, and the walk
// of the eight tables that decides who wears it — see the header for the
// geometry, why world axes, why the cube is hit and why one predicate.
#include <3d/place_marker.h>

#include "marker_lines.h"

#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/panel_component.h>
#include <3d/shape_component.h>
#include <3d/water_component.h>

#include <base/assert.h>

#include <ecs/component.h>

#include <math/float3.h>
#include <math/float4x4.h>

#include <scene/camera_component.h>
#include <scene/light_component.h>
#include <scene/point_light_component.h>
#include <scene/transform_component.h>

#include <math.h>
#include <stddef.h>

// The tips' distance from the position and the cube's half extent, metres
// (0365 point 1).
#define TIP 0.25f
#define HALF 0.25f

typedef voe_3d_marker_segment segment;

// A pose at `position` in the world's axes, unscaled: all that places a place.
static voe_scene_transform placed(voe_math_double3 position)
{
	return (voe_scene_transform){
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
}

// The octahedron's edges: each of the four equator tips (±X, ±Z) to the next
// round, and to each pole (±Y).
static void marker_edges(segment edges[VOE_3D_PLACE_MARKER_EDGES])
{
	const voe_math_float3 equator[4] = {
		{ TIP, 0.0f, 0.0f },
		{ 0.0f, 0.0f, TIP },
		{ -TIP, 0.0f, 0.0f },
		{ 0.0f, 0.0f, -TIP },
	};
	const voe_math_float3 up = { 0.0f, TIP, 0.0f };
	const voe_math_float3 down = { 0.0f, -TIP, 0.0f };
	uint32_t n = 0;

	for (uint32_t i = 0; i < 4; i++) {
		edges[n++] = (segment){ equator[i], equator[(i + 1) % 4] };
		edges[n++] = (segment){ equator[i], up };
		edges[n++] = (segment){ equator[i], down };
	}
	VOE_BASE_ASSERT(n == VOE_3D_PLACE_MARKER_EDGES,
			"the place marker's edge count drifted");
}

bool voe_3d_place_marker_quads(voe_math_double3 position, voe_render_view view,
			       voe_math_double3 eye, voe_platform_size size,
			       float pixels, voe_base_arena *arena,
			       voe_3d_outline_mesh *out)
{
	segment edges[VOE_3D_PLACE_MARKER_EDGES];

	VOE_BASE_ASSERT(arena != NULL, "a place marker with no arena");
	VOE_BASE_ASSERT(out != NULL, "a place marker into nothing");

	if (size.width == 0 || size.height == 0)
		return false;

	marker_edges(edges);
	// About the eye, as `view` is: the quads are eye-relative (ADR-0250).
	voe_3d_marker_lines(edges, VOE_3D_PLACE_MARKER_EDGES,
			    voe_scene_transform_matrix(placed(position), eye),
			    view, size, pixels, arena, out);
	return true;
}

bool voe_3d_place_marker_hit(voe_math_double3 position, voe_3d_ray ray,
			     float *distance)
{
	VOE_BASE_ASSERT(isfinite(ray.direction.x) && isfinite(ray.direction.y) &&
				isfinite(ray.direction.z),
			"a place hit along no direction");

	// About the ray's own origin, so only the small difference is narrowed
	// (ADR-0250).
	return voe_3d_marker_box_hit(
		voe_scene_transform_matrix(placed(position), ray.origin),
		(voe_math_float3){ HALF, HALF, HALF }, ray.direction, distance);
}

// Whether the world registered the store under `key`. A walk of the types
// rather than voe_ecs_component_type, which asserts on a key nothing
// registered, as pick.c's has_store walks them.
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

// Whether `entity` has a row in the table under `key`; none when unregistered.
static bool has_row(const voe_ecs_world *world, const struct voe_ecs_key *key,
		    voe_ecs_entity entity)
{
	return has_store(world, key) &&
	       voe_ecs_component_get(world, voe_ecs_component_type(world, key),
				     entity) != NULL;
}

bool voe_3d_place_marker_wanted(const voe_ecs_world *world,
				voe_ecs_entity entity)
{
	// Every table that already gives an entity a mesh or a marker of its own.
	static const struct voe_ecs_key *const worn[] = {
		&voe_3d_shape_key,     &voe_3d_model_key,
		&voe_3d_water_key,     &voe_3d_mesh_key,
		&voe_3d_panel_key,     &voe_scene_camera_key,
		&voe_scene_light_key,  &voe_scene_point_light_key,
	};

	VOE_BASE_ASSERT(world != NULL, "a place marker asked of no world");

	if (!voe_ecs_entity_alive(world, entity) ||
	    !has_row(world, &voe_scene_transform_key, entity))
		return false;
	for (uint32_t i = 0; i < sizeof worn / sizeof worn[0]; i++)
		if (has_row(world, worn[i], entity))
			return false;
	return true;
}
