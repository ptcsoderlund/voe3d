// The gizmo's size, the ray tests behind each handle, the point a drag is
// measured from and the triangles all of it is drawn as — see the header for why
// one module answers them all, why a handle is a camera-facing quad and why the
// marked one is a mesh of its own.
//
// An arrow is tested as the segment out to its head's far end and a square as
// its patch of plane; a drag is measured from the closest point on an axis or
// where the ray meets a plane; the meshes are shafts, heads, squares and label
// strokes, each quad and its winding gizmo_quads.c's.
#include <3d/gizmo.h>

#include "gizmo_quads.h"

#include <base/arena.h>
#include <base/assert.h>

#include <math/double3.h>
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

// A cross product this short is an arrow pointed so nearly at the eye, or a
// label seen so nearly along the world's up, that the direction across it cannot
// be worked out: normalising it would assert (math/float3.h) and the answer
// would jump between two frames.
#define DEGENERATE 1e-6f

// The three letters, as strokes in the label's own square: x across and y up,
// each from -1 to 1. X is two crossed strokes, Y a fork over a stem and Z two
// bars with a diagonal between them (ADR-0206).
static const uint32_t LETTER_STROKES[3] = { 2, 3, 3 };
static_assert(2 + 3 + 3 == VOE_3D_GIZMO_LABEL_STROKES,
	      "the letters below cost what the header says they do");
static const float LETTER[3][3][4] = {
	{ { -1.0f, -1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f, 1.0f, -1.0f } },
	{ { -1.0f, 1.0f, 0.0f, 0.0f },
	  { 1.0f, 1.0f, 0.0f, 0.0f },
	  { 0.0f, 0.0f, 0.0f, -1.0f } },
	{ { -1.0f, 1.0f, 1.0f, 1.0f },
	  { 1.0f, 1.0f, -1.0f, -1.0f },
	  { -1.0f, -1.0f, 1.0f, -1.0f } },
};

// How wide a letter is beside how tall, which is what keeps an X from reading as
// a cross and a Z from reading as an S.
#define LETTER_HALF_WIDTH 0.35f

// The world's three axes, in the order the handles name them. A plane handle
// `p` counting from VOE_3D_GIZMO_XY lies in axes p and (p + 1) % 3 and is
// normal to (p + 2) % 3, which is what makes XY, YZ and ZX one rule and not
// three.
// The gizmo and a ray as float about one double point (ADR-0250): the ray's
// origin for a hit or a grab, the eye for the quads. Every test and every
// vertex below is worked out in this, never in world positions.
struct flat {
	voe_math_float3 origin;
	voe_math_float3 eye;
	float shaft;
};

struct flat_ray {
	voe_math_float3 origin;
	voe_math_float3 direction;
};

// `gizmo` as offsets from `point`, subtracted in double and only then narrowed.
static struct flat flat_about(voe_3d_gizmo gizmo, voe_math_double3 point)
{
	VOE_BASE_ASSERT(gizmo.shaft >= 0.0f, "a gizmo of negative size");
	return (struct flat){
		.origin = voe_math_double3_to_float3(
			voe_math_double3_sub(gizmo.origin, point)),
		.eye = voe_math_double3_to_float3(
			voe_math_double3_sub(gizmo.eye, point)),
		.shaft = gizmo.shaft,
	};
}

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
static voe_math_float3 out_from(struct flat gizmo, int axis, float along)
{
	return voe_math_float3_add(gizmo.origin,
				   voe_math_float3_scale(AXES[axis], along));
}

// The point `along_first` metres along one of a plane's two axes and
// `along_second` along the other, from the gizmo's origin.
static voe_math_float3 corner(struct flat gizmo, voe_math_float3 first,
			      voe_math_float3 second, float along_first,
			      float along_second)
{
	return voe_math_float3_add(
		gizmo.origin,
		voe_math_float3_add(
			voe_math_float3_scale(first, along_first),
			voe_math_float3_scale(second, along_second)));
}

// A point of a letter's own square, `x` across the label and `y` up it, both
// from -1 to 1: the three letters are written once in that square and placed by
// this.
static voe_math_float3 in_label(voe_math_float3 centre, voe_math_float3 right,
				voe_math_float3 up, float height, float x,
				float y)
{
	return voe_math_float3_add(
		centre,
		voe_math_float3_add(
			voe_math_float3_scale(
				right, x * height * LETTER_HALF_WIDTH),
			voe_math_float3_scale(up, y * 0.5f * height)));
}

// Where `ray` meets the plane through `point` with unit normal `normal`, as a
// parameter along the ray. False when it is too nearly parallel to that plane to
// meet it anywhere in particular.
static bool ray_meets_plane(struct flat_ray ray, voe_math_float3 point,
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
static bool ray_near_segment(struct flat_ray ray, voe_math_float3 a,
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
static bool ray_hits_axis(struct flat gizmo, struct flat_ray ray, int axis,
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
static bool ray_hits_plane(struct flat gizmo, struct flat_ray ray, int plane,
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

voe_3d_gizmo voe_3d_gizmo_at(voe_math_double3 origin, voe_render_view view,
			     voe_math_double3 eye, voe_platform_size size,
			     float pixels)
{
	// `view` is about the eye, so the origin is too.
	float depth = depth_of(view, voe_math_double3_to_float3(
					     voe_math_double3_sub(origin, eye)));

	VOE_BASE_ASSERT(size.height > 0,
			"a gizmo sized against a picture of no height");
	VOE_BASE_ASSERT(pixels > 0.0f, "a gizmo asked to cover no pixels");
	VOE_BASE_ASSERT(view.projection.m[1][1] > 0.0f,
			"a projection with no vertical field of view");
	if (depth < NEAREST_DEPTH)
		depth = NEAREST_DEPTH;
	return (voe_3d_gizmo){
		.origin = origin,
		.eye = eye,
		.shaft = pixels * 2.0f * depth /
			 (view.projection.m[1][1] * (float)size.height),
	};
}

voe_3d_gizmo_handle voe_3d_gizmo_hit(voe_3d_gizmo placed, voe_3d_ray world_ray)
{
	struct flat gizmo = flat_about(placed, world_ray.origin);
	struct flat_ray ray = { { 0.0f, 0.0f, 0.0f }, world_ray.direction };
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

bool voe_3d_gizmo_grab(voe_3d_gizmo placed, voe_3d_gizmo_handle handle,
		       voe_3d_ray world_ray, voe_math_double3 *out)
{
	struct flat gizmo = flat_about(placed, world_ray.origin);
	struct flat_ray ray = { { 0.0f, 0.0f, 0.0f }, world_ray.direction };
	voe_math_float3 from_origin =
		voe_math_float3_sub(ray.origin, gizmo.origin);
	voe_math_float3 point;
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
		point = voe_math_float3_add(
			gizmo.origin,
			voe_math_float3_scale(
				axis,
				(voe_math_float3_dot(from_origin, axis) -
				 ray_along * voe_math_float3_dot(
						     from_origin,
						     ray.direction)) /
					determinant));
	} else if (!ray_meets_plane(ray, gizmo.origin,
				    AXES[(handle - VOE_3D_GIZMO_XY + 2) % 3],
				    &at)) {
		return false;
	} else {
		point = voe_math_float3_scale(ray.direction, at);
	}
	// Back into the world, added in double.
	*out = voe_math_double3_add(world_ray.origin,
				    voe_math_double3_from_float3(point));
	return true;
}

// The triangle of those three corners, which is an arrowhead and nothing else.
static void add_triangle(struct voe_3d_gizmo_build *mesh, voe_math_float3 a,
			 voe_math_float3 b, voe_math_float3 c,
			 voe_math_float3 eye)
{
	uint32_t v = mesh->vertex_count;

	VOE_BASE_ASSERT(mesh->vertex_count + 3 <= mesh->vertex_capacity,
			"more vertices than a gizmo has");
	VOE_BASE_ASSERT(mesh->vertices != NULL, "a triangle built nowhere");
	mesh->vertices[v + 0] = voe_3d_gizmo_facing(a, eye);
	mesh->vertices[v + 1] = voe_3d_gizmo_facing(b, eye);
	mesh->vertices[v + 2] = voe_3d_gizmo_facing(c, eye);
	mesh->vertex_count += 3;
	voe_3d_gizmo_wind(mesh, v + 0, v + 1, v + 2, eye);
}

// The unit an arrow is widened along: perpendicular to the axis and to the
// direction from the middle of the shaft to the eye, so the arrow keeps its face
// to the person. An arrow pointed almost at the eye leaves that cross product at
// nearly nothing, and a perpendicular to the axis is taken instead — a sliver
// rather than nothing at all.
static voe_math_float3 across_axis(struct flat gizmo, int axis)
{
	voe_math_float3 to_eye = voe_math_float3_sub(
		gizmo.eye, out_from(gizmo, axis, 0.5f * gizmo.shaft));
	voe_math_float3 across = voe_math_float3_cross(AXES[axis], to_eye);

	VOE_BASE_ASSERT(axis >= 0 && axis < 3, "an axis of no world");
	VOE_BASE_ASSERT(gizmo.shaft > 0.0f, "an arrow of no length");
	if (voe_math_float3_length(across) < DEGENERATE)
		return AXES[(axis + 1) % 3];
	return voe_math_float3_normalize(across);
}

// One arrow: the shaft's quad from the origin to the shaft's end, and the head's
// triangle from there to the point, both in the one plane that faces the eye.
// `step` widens the two without lengthening either, which is what marking is.
static void add_axis(struct voe_3d_gizmo_build *mesh, struct flat gizmo,
		     int axis, float step)
{
	voe_math_float3 across = across_axis(gizmo, axis);
	voe_math_float3 end = out_from(gizmo, axis, gizmo.shaft);
	voe_math_float3 point = out_from(
		gizmo, axis, gizmo.shaft * (1.0f + VOE_3D_GIZMO_HEAD_LENGTH));
	voe_math_float3 half = voe_math_float3_scale(
		across, gizmo.shaft * VOE_3D_GIZMO_HEAD_HALF_WIDTH * step);

	VOE_BASE_ASSERT(step > 0.0f, "an arrow of no width");
	VOE_BASE_ASSERT(mesh != NULL, "an arrow built nowhere");
	voe_3d_gizmo_add_quad(mesh, gizmo.origin, end, across,
			      gizmo.shaft * VOE_3D_GIZMO_LINE_HALF_WIDTH * step,
			      gizmo.eye);
	add_triangle(mesh, voe_math_float3_sub(end, half),
		     voe_math_float3_add(end, half), point, gizmo.eye);
}

// One plane square: the patch of the plane itself between the near corner and
// the far one on both of its axes, as two triangles wound from whichever side
// the eye is on. `step` grows it about its own centre, so a marked square still
// covers the middle it covered.
static void add_square(struct voe_3d_gizmo_build *mesh, struct flat gizmo,
		       int plane, float step)
{
	float near_corner = gizmo.shaft * VOE_3D_GIZMO_PLANE_NEAR;
	float side = gizmo.shaft * VOE_3D_GIZMO_PLANE_SIDE;
	float low = near_corner + 0.5f * side * (1.0f - step);
	float high = near_corner + 0.5f * side * (1.0f + step);
	voe_math_float3 first = AXES[plane];
	voe_math_float3 second = AXES[(plane + 1) % 3];
	uint32_t v = mesh->vertex_count;

	VOE_BASE_ASSERT(plane >= 0 && plane < 3, "a plane of no world");
	VOE_BASE_ASSERT(mesh->vertex_count + 4 <= VOE_3D_GIZMO_VERTICES,
			"more vertices than a gizmo has");
	mesh->vertices[v + 0] = voe_3d_gizmo_facing(
		corner(gizmo, first, second, low, low), gizmo.eye);
	mesh->vertices[v + 1] = voe_3d_gizmo_facing(
		corner(gizmo, first, second, high, low), gizmo.eye);
	mesh->vertices[v + 2] = voe_3d_gizmo_facing(
		corner(gizmo, first, second, high, high), gizmo.eye);
	mesh->vertices[v + 3] = voe_3d_gizmo_facing(
		corner(gizmo, first, second, low, high), gizmo.eye);
	mesh->vertex_count += 4;
	voe_3d_gizmo_wind(mesh, v + 0, v + 1, v + 2, gizmo.eye);
	voe_3d_gizmo_wind(mesh, v + 0, v + 2, v + 3, gizmo.eye);
}

// One label, past the point of its arrow: its strokes drawn in the plane facing
// the eye, upright against the world's up where there is one to be upright
// against. Each stroke is the quad a shaft is, widened in that same plane.
static void add_label(struct voe_3d_gizmo_build *mesh, struct flat gizmo,
		      int axis)
{
	float height = gizmo.shaft * VOE_3D_GIZMO_LABEL_HEIGHT;
	voe_math_float3 centre = out_from(
		gizmo, axis,
		gizmo.shaft * (1.0f + VOE_3D_GIZMO_HEAD_LENGTH) + height);
	voe_math_float3 to_eye = voe_math_float3_sub(gizmo.eye, centre);
	voe_math_float3 forward = voe_math_float3_normalize(to_eye);
	voe_math_float3 right = voe_math_float3_cross(AXES[1], forward);
	voe_math_float3 up;
	uint32_t stroke;

	VOE_BASE_ASSERT(axis >= 0 && axis < 3, "a label of no axis");
	VOE_BASE_ASSERT(LETTER_STROKES[axis] <= 3, "a letter of too many strokes");
	right = voe_math_float3_length(right) < DEGENERATE ?
			AXES[0] :
			voe_math_float3_normalize(right);
	up = voe_math_float3_cross(forward, right);
	for (stroke = 0; stroke < LETTER_STROKES[axis]; stroke++) {
		const float *ends = LETTER[axis][stroke];
		voe_math_float3 from = in_label(centre, right, up, height,
						ends[0], ends[1]);
		voe_math_float3 to = in_label(centre, right, up, height, ends[2],
					      ends[3]);

		voe_3d_gizmo_add_quad(
			mesh, from, to,
			voe_math_float3_normalize(voe_math_float3_cross(
				voe_math_float3_sub(to, from), forward)),
			gizmo.shaft * VOE_3D_GIZMO_LINE_HALF_WIDTH, gizmo.eye);
	}
}

bool voe_3d_gizmo_quads(voe_3d_gizmo placed, voe_3d_gizmo_handle marked,
			voe_base_arena *arena, voe_3d_gizmo_mesh *plain,
			voe_3d_gizmo_mesh *marked_out)
{
	// About the eye, so every vertex is eye-relative as the pass draws it.
	struct flat gizmo = flat_about(placed, placed.eye);
	struct voe_3d_gizmo_build at_rest;
	struct voe_3d_gizmo_build under;
	int handle;

	VOE_BASE_ASSERT(arena != NULL, "a gizmo built with no arena");
	VOE_BASE_ASSERT(plain != NULL && marked_out != NULL,
			"a gizmo built into nothing");
	VOE_BASE_ASSERT(marked <= VOE_3D_GIZMO_ZX, "a handle of no gizmo");
	VOE_BASE_ASSERT(gizmo.shaft >= 0.0f, "a gizmo of negative size");
	if (gizmo.shaft <= 0.0f)
		return false;

	// Each of the whole gizmo's size, because a marked handle is a few
	// vertices and one bound is fewer numbers than two.
	at_rest = voe_3d_gizmo_build_in(arena, VOE_3D_GIZMO_VERTICES,
					VOE_3D_GIZMO_INDICES);
	under = voe_3d_gizmo_build_in(arena, VOE_3D_GIZMO_VERTICES,
				      VOE_3D_GIZMO_INDICES);

	for (handle = 0; handle < 6; handle++) {
		bool is_marked = marked == (voe_3d_gizmo_handle)(
						   VOE_3D_GIZMO_X + handle);
		struct voe_3d_gizmo_build *mesh = is_marked ? &under : &at_rest;
		float step = is_marked ? VOE_3D_GIZMO_MARKED_STEP : 1.0f;

		if (handle < 3)
			add_axis(mesh, gizmo, handle, step);
		else
			add_square(mesh, gizmo, handle - 3, step);
	}
	// The labels say which arrow is which, which is as true of the marked
	// one as of the other two, so they are all in the mesh at rest.
	for (handle = 0; handle < 3; handle++)
		add_label(&at_rest, gizmo, handle);

	*plain = voe_3d_gizmo_mesh_of(at_rest);
	*marked_out = voe_3d_gizmo_mesh_of(under);
	return true;
}
