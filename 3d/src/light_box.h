// How the sun frames a box of the world: its basis, the snap of a centre to
// whole texels about the world origin, and the orthographic view looking at
// that centre. Internal to 3d: the shadow cascades (shadow_cascades.c) frame
// the sun with them.
//
//     struct voe_3d_light_basis basis = voe_3d_light_box_basis(direction);
//     centre = voe_3d_light_box_snap(centre, eye, basis, texel);
//     light = voe_3d_light_box_look(centre, half, radius, basis);
//
// THE BASIS DEPENDS ON THE SUN ALONE: forward is `direction`, right is forward
// × world up, up is right × forward; with the sun straight up or down world Z
// stands in for world up.
//
// THE SNAP IS IN DOUBLE ABOUT THE WORLD ORIGIN (0250): the centre is taken to
// eye + centre and rounded to whole `texel`s along right and up there, then
// returned eye-relative, so it is as fine 100 km out as at the origin.
//
// THE LOOK IS PULLED BACK radius + VOE_3D_SHADOW_CASTER_REACH toward the sun,
// its translation taken from the centre rather than the pulled-back eye, its
// box `half` a side across and up and reaching the sphere's far side.
//
// Arithmetic only. A direction that gives no basis, or a texel of no size,
// asserts.
#pragma once

#include <math/double3.h>
#include <math/float3.h>
#include <render/device.h>

struct voe_3d_light_basis {
	voe_math_float3 right;
	voe_math_float3 up;
	voe_math_float3 forward;
};

struct voe_3d_light_basis voe_3d_light_box_basis(voe_math_float3 direction);

voe_math_float3 voe_3d_light_box_snap(voe_math_float3 centre, voe_math_double3 eye,
				      struct voe_3d_light_basis basis, double texel);

voe_render_view voe_3d_light_box_look(voe_math_float3 centre, float half,
				      float radius, struct voe_3d_light_basis basis);
