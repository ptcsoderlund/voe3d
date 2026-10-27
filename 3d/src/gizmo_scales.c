// The scale gizmo's shafts and squares — see the header for why the entity's
// own axes, why the factor is measured from the press, why the centre reads
// travel up the plane facing the eye and why the factor has a floor.
//
// The centre square is tested first, where the ray crosses the plane through the
// origin facing the eye; an axis is the segment from the origin to its tip
// square's far side, met within the grip. A grab is that crossing or the axis
// line's nearest point; the meshes are a quad per shaft and per square, built
// and wound by gizmo_quads.c.
#include <3d/gizmo_scales.h>

#include "gizmo_quads.h"

#include <base/arena.h>
#include <base/assert.h>

#include <math/double3.h>
#include <math/float3.h>
#include <math/float4x4.h>

#include <math.h>
#include <stddef.h>

// A ray this nearly along an axis or in a plane meets it nowhere in particular;
// the sine of about a twentieth of a degree, gizmo.c's own bound. It is also the
// share of a shaft an axis press must be from the origin to scale from.
#define PARALLEL 1e-3f

// A cross product this short has no direction to normalise.
#define DEGENERATE 1e-6f

static const voe_math_float3 WORLD_UP = { 0.0f, 1.0f, 0.0f };
static const voe_math_float3 WORLD_FORWARD = { 0.0f, 0.0f, -1.0f };

// The gizmo as offsets from `point`, subtracted in double and then narrowed.
struct flat {
	voe_math_float3 origin;
	voe_math_float3 eye;
	float shaft;
};

static struct flat flat_about(voe_3d_gizmo gizmo, voe_math_double3 point)
{
	VOE_BASE_ASSERT(gizmo.shaft >= 0.0f, "a gizmo of negative size");
	VOE_BASE_ASSERT(isfinite(gizmo.shaft), "a gizmo of no finite size");
	return (struct flat){
		.origin = voe_math_double3_to_float3(
			voe_math_double3_sub(gizmo.origin, point)),
		.eye = voe_math_double3_to_float3(
			voe_math_double3_sub(gizmo.eye, point)),
		.shaft = gizmo.shaft,
	};
}

// The entity's own axis `axis`: that column of its rotation matrix.
static voe_math_float3 own_axis(voe_math_quat turn, int axis)
{
	voe_math_float4x4 m = voe_math_float4x4_from_quat(turn);

	VOE_BASE_ASSERT(axis >= 0 && axis < 3, "an axis of no entity");
	return (voe_math_float3){ m.m[0][axis], m.m[1][axis], m.m[2][axis] };
}

// A square facing the eye: towards the eye, and across and up it, up being the
// plane's direction nearest the world's up, or nearest -Z looking along Y.
struct facing {
	voe_math_float3 forward;
	voe_math_float3 right;
	voe_math_float3 up;
};

static struct facing facing_at(voe_math_float3 centre, voe_math_float3 eye)
{
	voe_math_float3 to_eye = voe_math_float3_sub(eye, centre);
	struct facing face = { .forward = { 0.0f, 0.0f, 1.0f } };

	if (voe_math_float3_length(to_eye) >= DEGENERATE)
		face.forward = voe_math_float3_normalize(to_eye);
	face.up = voe_math_float3_sub(
		WORLD_UP, voe_math_float3_scale(face.forward,
						voe_math_float3_dot(face.forward,
								    WORLD_UP)));
	if (voe_math_float3_length(face.up) < PARALLEL)
		face.up = voe_math_float3_sub(
			WORLD_FORWARD,
			voe_math_float3_scale(
				face.forward,
				voe_math_float3_dot(face.forward,
						    WORLD_FORWARD)));
	face.up = voe_math_float3_normalize(face.up);
	face.right = voe_math_float3_cross(face.up, face.forward);
	return face;
}

// Where a ray from nought along `direction` crosses the plane through `point`
// with unit normal `normal`, as a parameter along it; false when too nearly in it.
static bool crossing(voe_math_float3 point, voe_math_float3 normal,
		     voe_math_float3 direction, float *at)
{
	float facing = voe_math_float3_dot(direction, normal);

	VOE_BASE_ASSERT(at != NULL, "a crossing written nowhere");
	if (fabsf(facing) < PARALLEL)
		return false;
	*at = voe_math_float3_dot(point, normal) / facing;
	return true;
}

// How far along unit `axis` from `origin` the line comes nearest a ray from
// nought along `direction`; false when the ray runs too nearly along it. The
// line joining the two nearest points is perpendicular to both (gizmo.c).
static bool nearest_along(voe_math_float3 origin, voe_math_float3 axis,
			  voe_math_float3 direction, float *along)
{
	voe_math_float3 from_origin = voe_math_float3_neg(origin);
	float ray_along = voe_math_float3_dot(direction, axis);
	float determinant = 1.0f - ray_along * ray_along;

	VOE_BASE_ASSERT(along != NULL, "a nearest point written nowhere");
	if (determinant <= PARALLEL * PARALLEL)
		return false;
	*along = (voe_math_float3_dot(from_origin, axis) -
		  ray_along * voe_math_float3_dot(from_origin, direction)) /
		 determinant;
	return true;
}

// Whether a ray from nought along `direction` crosses the centre square, and
// how far along it.
static bool in_centre(struct flat gizmo, voe_math_float3 direction, float *at)
{
	struct facing face = facing_at(gizmo.origin, gizmo.eye);
	float half = gizmo.shaft * VOE_3D_GIZMO_SCALE_CENTRE_HALF;
	voe_math_float3 from_origin;

	if (!crossing(gizmo.origin, face.forward, direction, at))
		return false;
	from_origin = voe_math_float3_sub(voe_math_float3_scale(direction, *at),
					  gizmo.origin);
	return fabsf(voe_math_float3_dot(from_origin, face.right)) <= half &&
	       fabsf(voe_math_float3_dot(from_origin, face.up)) <= half;
}

// Whether a ray from nought along `direction` passes within the grip of the
// axis's segment, origin to the tip square's far side, in front of the eye.
static bool near_axis(struct flat gizmo, voe_math_float3 axis,
		      voe_math_float3 direction, float *at)
{
	float length = gizmo.shaft * (1.0f + VOE_3D_GIZMO_SCALE_TIP_HALF);
	float along = 0.0f;
	voe_math_float3 point;
	float on_ray;

	if (!nearest_along(gizmo.origin, axis, direction, &along))
		along = 0.0f;
	along = fminf(fmaxf(along, 0.0f), length);
	point = voe_math_float3_add(gizmo.origin,
				    voe_math_float3_scale(axis, along));
	on_ray = voe_math_float3_dot(point, direction);
	if (on_ray <= 0.0f)
		return false;
	*at = on_ray;
	return voe_math_float3_length(voe_math_float3_sub(
		       point, voe_math_float3_scale(direction, on_ray))) <=
	       gizmo.shaft * VOE_3D_GIZMO_GRIP;
}

voe_3d_gizmo_handle voe_3d_gizmo_scales_hit(voe_3d_gizmo placed,
					    voe_math_quat turn, voe_3d_ray ray)
{
	struct flat gizmo = flat_about(placed, ray.origin);
	voe_3d_gizmo_handle nearest = VOE_3D_GIZMO_NONE;
	float nearest_at = 0.0f;
	float at;
	int axis;

	VOE_BASE_ASSERT(voe_math_float3_length(ray.direction) > 0.5f,
			"a ray with no direction");
	if (gizmo.shaft <= 0.0f)
		return VOE_3D_GIZMO_NONE;
	if (in_centre(gizmo, ray.direction, &at))
		return at > 0.0f ? VOE_3D_GIZMO_CENTRE : VOE_3D_GIZMO_NONE;
	for (axis = 0; axis < 3; axis++)
		if (near_axis(gizmo, own_axis(turn, axis), ray.direction,
			      &at) &&
		    (nearest == VOE_3D_GIZMO_NONE || at < nearest_at)) {
			nearest = (voe_3d_gizmo_handle)(VOE_3D_GIZMO_X + axis);
			nearest_at = at;
		}
	return nearest;
}

bool voe_3d_gizmo_scales_grab(voe_3d_gizmo placed, voe_math_quat turn,
			      voe_3d_gizmo_handle handle, voe_3d_ray ray,
			      voe_math_double3 *out)
{
	struct flat gizmo = flat_about(placed, ray.origin);
	voe_math_float3 point;
	float at;

	VOE_BASE_ASSERT(out != NULL, "a grab written nowhere");
	VOE_BASE_ASSERT(handle <= VOE_3D_GIZMO_CENTRE, "a handle of no gizmo");
	if (handle == VOE_3D_GIZMO_CENTRE) {
		if (!crossing(gizmo.origin,
			      facing_at(gizmo.origin, gizmo.eye).forward,
			      ray.direction, &at))
			return false;
		point = voe_math_float3_scale(ray.direction, at);
	} else if (handle >= VOE_3D_GIZMO_X && handle <= VOE_3D_GIZMO_Z) {
		voe_math_float3 axis = own_axis(turn, handle - VOE_3D_GIZMO_X);

		if (!nearest_along(gizmo.origin, axis, ray.direction, &at))
			return false;
		point = voe_math_float3_add(gizmo.origin,
					    voe_math_float3_scale(axis, at));
	} else {
		return false;
	}
	// Back into the world, added in double.
	*out = voe_math_double3_add(ray.origin,
				    voe_math_double3_from_float3(point));
	return true;
}

float voe_3d_gizmo_scales_factor(voe_3d_gizmo placed, voe_math_quat turn,
				 voe_3d_gizmo_handle handle,
				 voe_math_double3 grab, voe_math_double3 now)
{
	// About the origin, so both grabs are offsets from it.
	struct flat gizmo = flat_about(placed, placed.origin);
	voe_math_float3 at_press = voe_math_double3_to_float3(
		voe_math_double3_sub(grab, placed.origin));
	voe_math_float3 at_now = voe_math_double3_to_float3(
		voe_math_double3_sub(now, placed.origin));
	float factor;

	VOE_BASE_ASSERT(handle <= VOE_3D_GIZMO_CENTRE, "a handle of no gizmo");
	if (gizmo.shaft <= 0.0f)
		return 1.0f;
	if (handle == VOE_3D_GIZMO_CENTRE) {
		factor = 1.0f + voe_math_float3_dot(
					voe_math_float3_sub(at_now, at_press),
					facing_at(gizmo.origin, gizmo.eye).up) /
					gizmo.shaft;
	} else if (handle >= VOE_3D_GIZMO_X && handle <= VOE_3D_GIZMO_Z) {
		voe_math_float3 axis = own_axis(turn, handle - VOE_3D_GIZMO_X);
		float pressed = voe_math_float3_dot(at_press, axis);

		if (fabsf(pressed) < PARALLEL * gizmo.shaft)
			return 1.0f;
		factor = voe_math_float3_dot(at_now, axis) / pressed;
	} else {
		return 1.0f;
	}
	return fmaxf(factor, VOE_3D_GIZMO_SCALE_LEAST);
}

// A square about `centre` facing the eye at nought, `half` to each side.
static void add_square(struct voe_3d_gizmo_build *mesh, voe_math_float3 centre,
		       float half)
{
	const voe_math_float3 eye = { 0.0f, 0.0f, 0.0f };
	struct facing face = facing_at(centre, eye);
	voe_math_float3 rise = voe_math_float3_scale(face.up, half);

	VOE_BASE_ASSERT(half > 0.0f, "a square of no size");
	voe_3d_gizmo_add_quad(mesh, voe_math_float3_sub(centre, rise),
			      voe_math_float3_add(centre, rise), face.right,
			      half, eye);
}

// One shaft from the origin to its tip, widened across itself towards the eye
// or along `aside` seen end on, and its tip square; `step` widens both.
static void add_axis(struct voe_3d_gizmo_build *mesh, struct flat gizmo,
		     voe_math_float3 axis, voe_math_float3 aside, float step)
{
	const voe_math_float3 eye = { 0.0f, 0.0f, 0.0f };
	voe_math_float3 tip = voe_math_float3_add(
		gizmo.origin, voe_math_float3_scale(axis, gizmo.shaft));
	voe_math_float3 across = voe_math_float3_cross(
		axis, voe_math_float3_neg(voe_math_float3_scale(
			      voe_math_float3_add(gizmo.origin, tip), 0.5f)));

	VOE_BASE_ASSERT(step > 0.0f, "a shaft of no width");
	across = voe_math_float3_length(across) < DEGENERATE ?
			 aside :
			 voe_math_float3_normalize(across);
	voe_3d_gizmo_add_quad(mesh, gizmo.origin, tip, across,
			      gizmo.shaft * VOE_3D_GIZMO_LINE_HALF_WIDTH * step,
			      eye);
	add_square(mesh, tip, gizmo.shaft * VOE_3D_GIZMO_SCALE_TIP_HALF * step);
}

bool voe_3d_gizmo_scales_quads(voe_3d_gizmo placed, voe_math_quat turn,
			       voe_3d_gizmo_handle marked,
			       voe_base_arena *arena, voe_3d_gizmo_mesh *plain,
			       voe_3d_gizmo_mesh *marked_out)
{
	// About the eye, so every vertex is eye-relative as the pass draws it.
	struct flat gizmo = flat_about(placed, placed.eye);
	struct voe_3d_gizmo_build at_rest;
	struct voe_3d_gizmo_build under;
	int axis;

	VOE_BASE_ASSERT(arena != NULL, "a scale gizmo built with no arena");
	VOE_BASE_ASSERT(plain != NULL && marked_out != NULL,
			"a scale gizmo built into nothing");
	VOE_BASE_ASSERT(marked <= VOE_3D_GIZMO_CENTRE, "a handle of no gizmo");
	if (gizmo.shaft <= 0.0f)
		return false;

	// Each of the whole gizmo's size, as gizmo.c's builds are.
	at_rest = voe_3d_gizmo_build_in(arena, VOE_3D_GIZMO_SCALE_VERTICES,
					VOE_3D_GIZMO_SCALE_INDICES);
	under = voe_3d_gizmo_build_in(arena, VOE_3D_GIZMO_SCALE_VERTICES,
				      VOE_3D_GIZMO_SCALE_INDICES);
	for (axis = 0; axis < 3; axis++) {
		bool is_marked =
			marked == (voe_3d_gizmo_handle)(VOE_3D_GIZMO_X + axis);

		add_axis(is_marked ? &under : &at_rest, gizmo,
			 own_axis(turn, axis), own_axis(turn, (axis + 1) % 3),
			 is_marked ? VOE_3D_GIZMO_MARKED_STEP : 1.0f);
	}
	if (marked == VOE_3D_GIZMO_CENTRE)
		add_square(&under, gizmo.origin,
			   gizmo.shaft * VOE_3D_GIZMO_SCALE_CENTRE_HALF *
				   VOE_3D_GIZMO_MARKED_STEP);
	else
		add_square(&at_rest, gizmo.origin,
			   gizmo.shaft * VOE_3D_GIZMO_SCALE_CENTRE_HALF);
	*plain = voe_3d_gizmo_mesh_of(at_rest);
	*marked_out = voe_3d_gizmo_mesh_of(under);
	return true;
}
