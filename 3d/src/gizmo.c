// The gizmo's size, the ray tests behind each handle and the point a drag is
// measured from — see the header for why one module answers all three, what "in
// front of the eye" means and why a refused grab is not a failure.
#include <3d/gizmo.h>

#include <base/assert.h>

#include <math/float3.h>
#include <math/float4x4.h>

#include <stddef.h>

// A gizmo at the eye itself would divide by nothing. A tenth of a millimetre is
// nearer than any near plane this engine is used with, so a floor there changes
// no gizmo anybody can see.
#define NEAREST_DEPTH 1e-4f

// A ray this nearly along an axis, or this nearly in a plane, has no one point
// on it worth calling the answer: the divisions below would be by nearly
// nothing and the point would jump metres between two frames. Directions are
// unit length, so this is the sine of about a twentieth of a degree.
#define PARALLEL 1e-3f

// The world's three axes, in the order the handles name them. A plane handle
// `p` counting from VOE_3D_GIZMO_XY lies in axes p and (p + 1) % 3 and is
// normal to (p + 2) % 3, which is what makes XY, YZ and ZX one rule and not
// three.
static const voe_math_float3 AXES[3] = { { 1.0f, 0.0f, 0.0f },
					 { 0.0f, 1.0f, 0.0f },
					 { 0.0f, 0.0f, 1.0f } };

// How far in front of the eye `p` is, in metres: the view matrix's third row is
// view-space Z, which runs backwards out of the screen (3d/outline.h says the
// same of an outline's depth).
static float depth_of(voe_render_view view, voe_math_float3 p)
{
	return -(view.view.m[2][0] * p.x + view.view.m[2][1] * p.y +
		 view.view.m[2][2] * p.z + view.view.m[2][3]);
}

// The point `along` metres along `axis` from the gizmo's origin.
static voe_math_float3 out_from(voe_3d_gizmo gizmo, int axis, float along)
{
	return voe_math_float3_add(gizmo.origin,
				   voe_math_float3_scale(AXES[axis], along));
}

// Where `ray` meets the plane through `point` with unit normal `normal`, as a
// parameter along the ray. False when it is too nearly parallel to that plane to
// meet it anywhere in particular.
static bool ray_meets_plane(voe_3d_ray ray, voe_math_float3 point,
			    voe_math_float3 normal, float *at)
{
	float facing = voe_math_float3_dot(ray.direction, normal);

	VOE_BASE_ASSERT(at != NULL, "a plane meeting written nowhere");
	if (facing > -PARALLEL && facing < PARALLEL)
		return false;
	*at = voe_math_float3_dot(voe_math_float3_sub(point, ray.origin),
				  normal) /
	      facing;
	return true;
}

// Whether `ray` passes within `reach` of the segment from `a` to `b`, and how
// far along the ray that happens.
//
// The closest approach of two lines is where the line joining them is
// perpendicular to both: with u = b - a and w = ray.origin - a, that is two
// equations whose determinant is u.u - (direction.u)^2, because the direction is
// unit length. A ray parallel to the segment leaves that determinant at nothing
// and every point of the segment equally close, so the near end is taken. The
// parameter on the segment is then clamped into it — past an end the closest
// point is that end — and the one on the ray is measured from the segment point
// that won, which is what keeps a hit behind the eye out.
static bool ray_near_segment(voe_3d_ray ray, voe_math_float3 a,
			     voe_math_float3 b, float reach, float *at)
{
	voe_math_float3 along = voe_math_float3_sub(b, a);
	voe_math_float3 from_start = voe_math_float3_sub(ray.origin, a);
	float length2 = voe_math_float3_dot(along, along);
	float ray_along = voe_math_float3_dot(ray.direction, along);
	float determinant = length2 - ray_along * ray_along;
	float on_segment = 0.0f;
	voe_math_float3 point;
	voe_math_float3 closest;
	float on_ray;

	VOE_BASE_ASSERT(length2 > 0.0f, "a handle segment of no length");
	VOE_BASE_ASSERT(reach > 0.0f, "a handle nothing can be within");
	if (determinant > PARALLEL * PARALLEL * length2)
		on_segment = (voe_math_float3_dot(along, from_start) -
			      ray_along * voe_math_float3_dot(ray.direction,
							      from_start)) /
			     determinant;
	if (on_segment < 0.0f)
		on_segment = 0.0f;
	if (on_segment > 1.0f)
		on_segment = 1.0f;
	point = voe_math_float3_add(a,
				    voe_math_float3_scale(along, on_segment));
	on_ray = voe_math_float3_dot(voe_math_float3_sub(point, ray.origin),
				     ray.direction);
	if (on_ray <= 0.0f)
		return false;
	closest = voe_math_float3_add(
		ray.origin, voe_math_float3_scale(ray.direction, on_ray));
	*at = on_ray;
	return voe_math_float3_length(voe_math_float3_sub(point, closest)) <=
	       reach;
}

// The arrow: the segment from the origin to the far end of the head, which is
// the whole of what is drawn along that axis.
static bool ray_hits_axis(voe_3d_gizmo gizmo, voe_3d_ray ray, int axis,
			  float *at)
{
	return ray_near_segment(ray, gizmo.origin,
				out_from(gizmo, axis,
					 gizmo.shaft *
						 (1.0f +
						  VOE_3D_GIZMO_HEAD_LENGTH)),
				gizmo.shaft * VOE_3D_GIZMO_GRIP, at);
}

// The plane square: where the ray meets that plane, kept when it lands between
// the near corner and the far one along both of the plane's own axes.
static bool ray_hits_plane(voe_3d_gizmo gizmo, voe_3d_ray ray, int plane,
			   float *at)
{
	float near_corner = gizmo.shaft * VOE_3D_GIZMO_PLANE_NEAR;
	float far_corner = near_corner + gizmo.shaft * VOE_3D_GIZMO_PLANE_SIDE;
	voe_math_float3 from_origin;
	float first;
	float second;

	if (!ray_meets_plane(ray, gizmo.origin, AXES[(plane + 2) % 3], at) ||
	    *at <= 0.0f)
		return false;
	from_origin = voe_math_float3_sub(
		voe_math_float3_add(ray.origin,
				    voe_math_float3_scale(ray.direction, *at)),
		gizmo.origin);
	first = voe_math_float3_dot(from_origin, AXES[plane]);
	second = voe_math_float3_dot(from_origin, AXES[(plane + 1) % 3]);
	return first >= near_corner && first <= far_corner &&
	       second >= near_corner && second <= far_corner;
}

voe_3d_gizmo voe_3d_gizmo_at(voe_math_float3 origin, voe_render_view view,
			     voe_platform_size size, float pixels)
{
	float depth = depth_of(view, origin);

	VOE_BASE_ASSERT(size.height > 0,
			"a gizmo sized against a picture of no height");
	VOE_BASE_ASSERT(pixels > 0.0f, "a gizmo asked to cover no pixels");
	VOE_BASE_ASSERT(view.projection.m[1][1] > 0.0f,
			"a projection with no vertical field of view");
	if (depth < NEAREST_DEPTH)
		depth = NEAREST_DEPTH;
	return (voe_3d_gizmo){
		.origin = origin,
		.eye = view.eye,
		.shaft = pixels * 2.0f * depth /
			 (view.projection.m[1][1] * (float)size.height),
	};
}

voe_3d_gizmo_handle voe_3d_gizmo_hit(voe_3d_gizmo gizmo, voe_3d_ray ray)
{
	voe_3d_gizmo_handle nearest = VOE_3D_GIZMO_NONE;
	float nearest_at = 0.0f;
	int handle;

	VOE_BASE_ASSERT(gizmo.shaft >= 0.0f, "a gizmo of negative size");
	VOE_BASE_ASSERT(voe_math_float3_length(ray.direction) > 0.5f,
			"a ray with no direction");
	if (gizmo.shaft <= 0.0f)
		return VOE_3D_GIZMO_NONE;

	for (handle = 0; handle < 6; handle++) {
		float at;
		bool met = handle < 3 ?
				   ray_hits_axis(gizmo, ray, handle, &at) :
				   ray_hits_plane(gizmo, ray, handle - 3, &at);

		if (met && (nearest == VOE_3D_GIZMO_NONE || at < nearest_at)) {
			nearest = (voe_3d_gizmo_handle)(VOE_3D_GIZMO_X +
							handle);
			nearest_at = at;
		}
	}
	return nearest;
}

bool voe_3d_gizmo_grab(voe_3d_gizmo gizmo, voe_3d_gizmo_handle handle,
		       voe_3d_ray ray, voe_math_float3 *out)
{
	voe_math_float3 from_origin =
		voe_math_float3_sub(ray.origin, gizmo.origin);
	float at;

	VOE_BASE_ASSERT(out != NULL, "a grab written nowhere");
	VOE_BASE_ASSERT(handle <= VOE_3D_GIZMO_ZX, "a handle of no gizmo");
	if (handle == VOE_3D_GIZMO_NONE)
		return false;
	if (handle <= VOE_3D_GIZMO_Z) {
		voe_math_float3 axis = AXES[handle - VOE_3D_GIZMO_X];
		float ray_along = voe_math_float3_dot(ray.direction, axis);
		float determinant = 1.0f - ray_along * ray_along;

		if (determinant <= PARALLEL * PARALLEL)
			return false;
		*out = voe_math_float3_add(
			gizmo.origin,
			voe_math_float3_scale(
				axis,
				(voe_math_float3_dot(from_origin, axis) -
				 ray_along * voe_math_float3_dot(
						     from_origin,
						     ray.direction)) /
					determinant));
		return true;
	}
	if (!ray_meets_plane(ray, gizmo.origin,
			     AXES[(handle - VOE_3D_GIZMO_XY + 2) % 3], &at))
		return false;
	*out = voe_math_float3_add(ray.origin,
				   voe_math_float3_scale(ray.direction, at));
	return true;
}
