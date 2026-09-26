// The sun's four shadow cascades fitted to one view, on the CPU (ADR-0258): the
// record a camera pass reads them through and the four light views their shadow
// passes draw with. Arithmetic only; no device, no allocation.
//
// A CASCADE IS ONE SLICE OF THE VIEW'S DEPTH SEEN FROM THE SUN. The view is cut
// at four rising distances; cascade i covers the slice between split i-1 (the
// near plane for the first) and split i, drawn into its own map, so the near
// slices get fine texels and the far ones coarse.
//
// NEAR AND FAR ARE READ BACK OUT OF THE VIEW'S PROJECTION (voe_3d_projection's
// m[2][2] = n / (f - n) and m[2][3] = n f / (f - n)), so the caller hands the
// view it draws with and nothing that could disagree with it.
//
// THE SPLITS ARE THE PRACTICAL SCHEME: each is VOE_3D_SHADOW_SPLIT_BLEND of the
// way from the even split to the logarithmic one, from the near plane to the
// lesser of the far plane and VOE_3D_SHADOW_REACH. Past the last, nothing is
// shadowed.
//
// EACH SLICE IS BOUNDED BY A SPHERE, AND ITS RADIUS DOES NOT TURN WITH THE VIEW.
// A box fitted to the slice's corners changes size as the camera turns, and a
// map whose texels change size shimmers. The sphere is taken in the view's own
// space, where the slice is the same whichever way the camera faces, and its
// radius is rounded up to a centimetre so float noise cannot change it.
//
// THE CENTRE SNAPS TO WHOLE TEXELS, IN DOUBLE, ABOUT THE WORLD ORIGIN (0250).
// Moving the box by a fraction of a texel resamples every edge and they crawl;
// moving it by whole texels moves the map under a fixed world, so shadows stand
// still. The centre is taken to double world space (eye + centre) and projected
// onto the light's two axes there, so the snap is as fine 100 km out as at the
// origin; the result goes back to eye-relative as a small float.
//
// THE BOX REACHES VOE_3D_SHADOW_CASTER_REACH FURTHER TOWARD THE SUN. A tree
// outside the slice still shadows into it; the light's view is pulled back that
// far past the sphere and its box runs from there to the sphere's far side.
//
// THE LIGHT'S BASIS DEPENDS ON THE SUN ALONE: forward is `direction`, right is
// forward × world up, up is right × forward. With the sun straight up or down
// forward × Y is nothing, so world Z stands in for world up.
//
// EVERY MATRIX IS EYE-RELATIVE LIKE `view`: it takes a position about `eye`, as
// every object's matrix is (0250). `light[i].eye` is the light's own position
// about the camera's eye; the shadow pass has no fragment stage to read it.
//
// `direction` is where the light goes, unit length, as voe_render_light's; a
// view whose projection is not voe_3d_projection's, or `texels` under two,
// asserts.
#pragma once

#include <math/double3.h>
#include <math/float3.h>
#include <render/device.h>

#include <stdint.h>

// The side of one cascade's map: what a program opens its device's
// `shadow_size` with.
#define VOE_3D_SHADOW_TEXELS 2048u
// How far from the eye shadows reach, in metres, at most.
#define VOE_3D_SHADOW_REACH 500.0f
// How far each split sits from the even one toward the logarithmic one.
#define VOE_3D_SHADOW_SPLIT_BLEND 0.9f
// How far past a slice toward the sun a caster is still drawn, in metres.
#define VOE_3D_SHADOW_CASTER_REACH 200.0f

typedef struct {
	voe_render_shadow shadow;
	voe_render_view light[VOE_RENDER_SHADOW_CASCADES];
} voe_3d_shadow_cascades;

voe_3d_shadow_cascades voe_3d_shadow_cascades_fit(voe_render_view view,
						  voe_math_double3 eye,
						  voe_math_float3 direction,
						  uint32_t texels);
