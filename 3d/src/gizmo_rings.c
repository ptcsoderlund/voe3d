// The rotate gizmo's rings — see the header for why world axes, why the angle
// is measured and not a delta, and why a ring is quads facing the eye.
//
// A ring is hit and measured where the ray crosses its plane: that crossing's
// distance from the origin against the radius, its direction in the plane as
// an angle. The meshes are one camera-facing quad per segment, built and wound
// by gizmo_quads.c.
#include <3d/gizmo_rings.h>

#include "gizmo_quads.h"

#include <base/arena.h>
#include <base/assert.h>

#include <math/double3.h>
#include <math/float3.h>

#include <math.h>
#include <stddef.h>

#define PI 3.14159265358979323846f

// A ray this nearly in a ring's plane crosses it nowhere in particular: the
// division below would be by nearly nothing and the angle would jump between
// two frames. Directions are unit length, so this is the sine of about a
// twentieth of a degree, gizmo.c's own bound.
#define PARALLEL 1e-3f

// A segment seen this nearly end on from the eye has no direction across it to
// widen along, and normalising the cross product would assert.
#define DEGENERATE 1e-6f

// The world's axes. Ring `a` lies in axes (a + 1) % 3 and (a + 2) % 3, in that
// order, which is what makes each ring's angle right-handed about its own axis.
static const voe_math_float3 AXES[3] = { { 1.0f, 0.0f, 0.0f },
					 { 0.0f, 1.0f, 0.0f },
					 { 0.0f, 0.0f, 1.0f } };

// The gizmo's origin about `point`, subtracted in double and then narrowed.
static voe_math_float3 origin_about(voe_3d_gizmo gizmo, voe_math_double3 point)
{
	VOE_BASE_ASSERT(gizmo.shaft >= 0.0f, "a gizmo of negative size");
	VOE_BASE_ASSERT(isfinite(gizmo.shaft), "a gizmo of no finite size");
	return voe_math_double3_to_float3(
		voe_math_double3_sub(gizmo.origin, point));
}

// Where a ray from nought along `direction` crosses ring `axis`'s plane
// through `origin`: `*at` along the ray and `*from_origin` the crossing less
// the origin. False when the ray lies too nearly in the plane.
static bool crossing(voe_math_float3 origin, voe_math_float3 direction,
		     int axis, float *at, voe_math_float3 *from_origin)
{
	float facing = voe_math_float3_dot(direction, AXES[axis]);

	VOE_BASE_ASSERT(axis >= 0 && axis < 3, "a ring of no world axis");
	VOE_BASE_ASSERT(at != NULL && from_origin != NULL,
			"a crossing written nowhere");
	if (facing > -PARALLEL && facing < PARALLEL)
		return false;
	*at = voe_math_float3_dot(origin, AXES[axis]) / facing;
	*from_origin = voe_math_float3_sub(
		voe_math_float3_scale(direction, *at), origin);
	return true;
}

voe_3d_gizmo_handle voe_3d_gizmo_rings_hit(voe_3d_gizmo gizmo, voe_3d_ray ray)
{
	voe_math_float3 origin = origin_about(gizmo, ray.origin);
	float grip = gizmo.shaft * VOE_3D_GIZMO_GRIP;
	voe_3d_gizmo_handle nearest = VOE_3D_GIZMO_NONE;
	float nearest_at = 0.0f;
	int axis;

	VOE_BASE_ASSERT(voe_math_float3_length(ray.direction) > 0.5f,
			"a ray with no direction");
	if (gizmo.shaft <= 0.0f)
		return VOE_3D_GIZMO_NONE;
	for (axis = 0; axis < 3; axis++) {
		voe_math_float3 from_origin;
		float at;

		if (!crossing(origin, ray.direction, axis, &at, &from_origin) ||
		    at <= 0.0f)
			continue;
		if (fabsf(voe_math_float3_length(from_origin) - gizmo.shaft) >
		    grip)
			continue;
		if (nearest == VOE_3D_GIZMO_NONE || at < nearest_at) {
			nearest = (voe_3d_gizmo_handle)(VOE_3D_GIZMO_X + axis);
			nearest_at = at;
		}
	}
	return nearest;
}

bool voe_3d_gizmo_rings_angle(voe_3d_gizmo gizmo, voe_3d_gizmo_handle handle,
			      voe_3d_ray ray, float *out)
{
	voe_math_float3 origin = origin_about(gizmo, ray.origin);
	voe_math_float3 from_origin;
	float at;
	int axis;

	VOE_BASE_ASSERT(out != NULL, "an angle written nowhere");
	VOE_BASE_ASSERT(handle <= VOE_3D_GIZMO_ZX, "a handle of no gizmo");
	if (handle == VOE_3D_GIZMO_NONE || handle > VOE_3D_GIZMO_Z)
		return false;
	axis = handle - VOE_3D_GIZMO_X;
	if (!crossing(origin, ray.direction, axis, &at, &from_origin))
		return false;
	*out = atan2f(voe_math_float3_dot(from_origin, AXES[(axis + 2) % 3]),
		      voe_math_float3_dot(from_origin, AXES[(axis + 1) % 3]));
	return true;
}

// The point of ring `axis` at `angle`, about whatever `origin` is about.
static voe_math_float3 on_ring(voe_math_float3 origin, float radius, int axis,
			       float angle)
{
	VOE_BASE_ASSERT(axis >= 0 && axis < 3, "a ring of no world axis");
	VOE_BASE_ASSERT(radius > 0.0f, "a ring of no radius");
	return voe_math_float3_add(
		origin,
		voe_math_float3_add(
			voe_math_float3_scale(AXES[(axis + 1) % 3],
					      radius * cosf(angle)),
			voe_math_float3_scale(AXES[(axis + 2) % 3],
					      radius * sinf(angle))));
}

// One ring about the eye at nought: each segment's quad widened across it
// towards the eye, or across the ring's own axis where the eye is end on.
static void add_ring(struct voe_3d_gizmo_build *mesh, voe_math_float3 origin,
		     float radius, int axis, float half_width)
{
	const voe_math_float3 eye = { 0.0f, 0.0f, 0.0f };
	uint32_t segment;

	VOE_BASE_ASSERT(half_width > 0.0f, "a ring of no width");
	VOE_BASE_ASSERT(mesh != NULL, "a ring built nowhere");
	for (segment = 0; segment < VOE_3D_GIZMO_RING_SEGMENTS; segment++) {
		float step = 2.0f * PI / (float)VOE_3D_GIZMO_RING_SEGMENTS;
		voe_math_float3 a =
			on_ring(origin, radius, axis, (float)segment * step);
		voe_math_float3 b = on_ring(origin, radius, axis,
					    (float)(segment + 1) * step);
		voe_math_float3 middle =
			voe_math_float3_scale(voe_math_float3_add(a, b), 0.5f);
		voe_math_float3 across = voe_math_float3_cross(
			voe_math_float3_normalize(voe_math_float3_sub(b, a)),
			voe_math_float3_neg(middle));

		across = voe_math_float3_length(across) < DEGENERATE ?
				 AXES[axis] :
				 voe_math_float3_normalize(across);
		voe_3d_gizmo_add_quad(mesh, a, b, across, half_width, eye);
	}
}

bool voe_3d_gizmo_rings_quads(voe_3d_gizmo gizmo, voe_3d_gizmo_handle marked,
			      voe_base_arena *arena, voe_3d_gizmo_mesh *plain,
			      voe_3d_gizmo_mesh *marked_out)
{
	// About the eye, so every vertex is eye-relative as the pass draws it.
	voe_math_float3 origin = origin_about(gizmo, gizmo.eye);
	float half_width = gizmo.shaft * VOE_3D_GIZMO_LINE_HALF_WIDTH;
	struct voe_3d_gizmo_build at_rest;
	struct voe_3d_gizmo_build under;
	int axis;

	VOE_BASE_ASSERT(arena != NULL, "rings built with no arena");
	VOE_BASE_ASSERT(plain != NULL && marked_out != NULL,
			"rings built into nothing");
	VOE_BASE_ASSERT(marked <= VOE_3D_GIZMO_ZX, "a handle of no gizmo");
	if (gizmo.shaft <= 0.0f)
		return false;

	// Each of all three rings' size, as gizmo.c's builds are.
	at_rest = voe_3d_gizmo_build_in(arena, VOE_3D_GIZMO_RING_VERTICES,
					VOE_3D_GIZMO_RING_INDICES);
	under = voe_3d_gizmo_build_in(arena, VOE_3D_GIZMO_RING_VERTICES,
				      VOE_3D_GIZMO_RING_INDICES);
	for (axis = 0; axis < 3; axis++) {
		if (marked == (voe_3d_gizmo_handle)(VOE_3D_GIZMO_X + axis))
			add_ring(&under, origin, gizmo.shaft, axis,
				 half_width * VOE_3D_GIZMO_MARKED_STEP);
		else
			add_ring(&at_rest, origin, gizmo.shaft, axis,
				 half_width);
	}
	*plain = voe_3d_gizmo_mesh_of(at_rest);
	*marked_out = voe_3d_gizmo_mesh_of(under);
	return true;
}
