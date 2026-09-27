// The element path's arithmetic, which needs no device: no graphics card, no
// headless device and no window, so every claim here runs on every machine.
//
// - the transform puts element (0,0) at the top-left of the screen;
// - a screen-filling surface stops short of the near clip boundary (bug 001,
//   ADR-0111), which no picture from this machine could show;
// - the surface matrix takes millimetres into metres, centred, y up;
// - the surface size divides both axes by the one scale;
// - `ui` scale at two halves the millimetres;
// - the element record is still eighty bytes, its fields where the shader
//   reads them.
//
// Each is where a direction or a size is decided, so it is asserted there
// rather than inferred from what tests/elements.c sees in a picture.
#include <render/device.h>

#include <math/float2.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stddef.h>
#include <stdio.h>

// The surface matrix, on its own and without a graphics card: millimetres in,
// metres out, the surface centred on its own origin and its top edge at +y.
//
// IT IS THE HALF OF THE ELEMENT PATH A PANEL IN THE WORLD USES, AND THE HALF
// THAT OWNS THE SIGN. A panel whose content is upside down is this row
// positive; a panel that looks right because something else negated Y as well
// is the expensive version of the same bug, and the only defence against it is
// that this is the one function with a minus sign in it.
static void the_surface_matrix_is_millimetres_into_metres(void)
{
	// A 240 by 135 mm surface, which is 0.24 by 0.135 metres.
	voe_math_float2 size = { 240.0f, 135.0f };
	voe_math_float4x4 m = voe_render_element_surface_matrix(size);
	voe_math_float4 top_left = voe_math_float4x4_mul_float4(
		m, (voe_math_float4){ 0.0f, 0.0f, 0.0f, 1.0f });
	voe_math_float4 bottom_right = voe_math_float4x4_mul_float4(
		m, (voe_math_float4){ 240.0f, 135.0f, 0.0f, 1.0f });
	voe_math_float4 middle = voe_math_float4x4_mul_float4(
		m, (voe_math_float4){ 120.0f, 67.5f, 0.0f, 1.0f });

	// Element (0,0) is the surface's top-left: left of the origin and ABOVE
	// it, because element y runs down and the world's runs up.
	VOE_TEST_CHECK_FLOAT(top_left.x, -0.12f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(top_left.y, 0.0675f, 1e-6f);
	// The far corner is right of the origin and below it.
	VOE_TEST_CHECK_FLOAT(bottom_right.x, 0.12f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(bottom_right.y, -0.0675f, 1e-6f);
	// The middle of the surface is the origin, which is what "centred on its
	// own origin" means and what makes the size a field on the component.
	VOE_TEST_CHECK_FLOAT(middle.x, 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(middle.y, 0.0f, 1e-6f);
	// Flat, and not projected by anything in here.
	VOE_TEST_CHECK_FLOAT(top_left.z, 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(top_left.w, 1.0f, 1e-6f);
}

// How many millimetres a target holds, without a graphics card: the pixel size
// divided by the scale, on both axes by the one number.
//
// THE CLAIM IS THAT NOTHING IS DEFORMED, AND DIVIDING BOTH AXES BY ONE NUMBER IS
// THE WHOLE OF IT. Two independent scales are what made a window of an unexpected
// shape stretch everything on it; this asserts that there is one, by checking a
// target whose aspect is nothing like the surface's authored one.
static void the_surface_size_divides_both_axes_by_one_scale(void)
{
	voe_platform_size wide = { 1920, 1080 };
	// A tall, narrow window — nothing like the 16:9 the surface was authored
	// at, which is the case that used to deform.
	voe_platform_size narrow = { 600, 1080 };
	voe_math_float2 mm = voe_render_element_surface_size(wide, 8.0f);
	voe_math_float2 half_scale = voe_render_element_surface_size(wide, 4.0f);
	voe_math_float2 tall = voe_render_element_surface_size(narrow, 8.0f);

	VOE_TEST_CHECK_FLOAT(mm.x, 240.0f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(mm.y, 135.0f, 1e-3f);
	// Halving the scale doubles both dimensions: fewer pixels to a
	// millimetre means more millimetres in the same window.
	VOE_TEST_CHECK_FLOAT(half_scale.x, 480.0f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(half_scale.y, 270.0f, 1e-3f);
	// And a window a third as wide holds a third of the millimetres across
	// and exactly as many down. That second half is the claim: the height
	// did not change, so nothing on the surface changed size or shape — what
	// changed is how much room there is beside it.
	VOE_TEST_CHECK_FLOAT(tall.x, 75.0f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(tall.y, 135.0f, 1e-3f);
}

// The calibration, asserted rather than eyeballed: the formula a program that
// wants a surface a fixed number of millimetres tall computes, and what
// ui_scale does to it.
//
// IT IS THE CALLER'S ARITHMETIC AND NOT THIS FOLDER'S, which is exactly why it
// is worth a test here. `render` takes pixels per millimetre and holds no
// policy; the one multiplication that turns "a proportion of the window" into
// that number lives at the call site, and if it is wrong nothing in this folder
// can tell. So the sum is written out once, where it can be checked.
static void ui_scale_at_two_halves_the_millimetres(void)
{
	voe_platform_size target = { 1920, 1080 };
	// What a surface authored 135 mm tall asks for: enough pixels per
	// millimetre that the window's height is exactly those 135 millimetres.
	float authored_high = 135.0f;
	float plain = (float)target.height / authored_high * 1.0f;
	float doubled = (float)target.height / authored_high * 2.0f;
	voe_math_float2 at_one = voe_render_element_surface_size(target, plain);
	voe_math_float2 at_two = voe_render_element_surface_size(target,
								doubled);

	// At a scale of one the surface is exactly as tall as it was authored,
	// whatever the window's shape, and as wide as the window's shape makes
	// it.
	VOE_TEST_CHECK_FLOAT(at_one.y, 135.0f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(at_one.x, 240.0f, 1e-3f);
	// At two, the surface holds half the millimetres — which is everything
	// on it drawn twice the size, with half as much room to put it in. That
	// is the whole of what the knob does.
	VOE_TEST_CHECK_FLOAT(at_two.y, 67.5f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(at_two.x, 120.0f, 1e-3f);
}

// The transform, on its own and without a graphics card: the three corners of
// element space that say which way round it is. It runs whether or not there is
// a Vulkan device, because it is arithmetic.
//
// IT IS CHECKED HERE RATHER THAN INFERRED FROM THE PICTURE because the picture
// is what the whole pipeline did and this is the one line that decides the
// direction. Both together say the direction is right and that nothing else
// undid it.
static void the_transform_puts_the_origin_at_the_top_left(void)
{
	voe_math_float4x4 m = voe_render_element_transform(
		(voe_math_float2){ 100.0f, 50.0f });
	voe_math_float4 origin = voe_math_float4x4_mul_float4(
		m, (voe_math_float4){ 0.0f, 0.0f, 0.0f, 1.0f });
	voe_math_float4 far_corner = voe_math_float4x4_mul_float4(
		m, (voe_math_float4){ 100.0f, 50.0f, 0.0f, 1.0f });

	// (0,0) millimetres is the left edge and the TOP of the screen, and with
	// the engine's negative viewport height the top of the screen is clip
	// y = +1. That +1 is the Y-down convention and there is no other flip.
	VOE_TEST_CHECK_FLOAT(origin.x, -1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(origin.y, 1.0f, 1e-5f);
	// The far corner is the right edge and the bottom.
	VOE_TEST_CHECK_FLOAT(far_corner.x, 1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(far_corner.y, -1.0f, 1e-5f);
	// Just inside the near plane, because depth runs backwards here and the
	// surface deliberately does not stand on the boundary — see
	// the_surface_stops_short_of_the_near_clip_boundary below, which is the
	// claim; this is the constant that satisfies it. w is one because an
	// element surface is not projected.
	VOE_TEST_CHECK_FLOAT(origin.z, 0.9999f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(origin.w, 1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(far_corner.w, 1.0f, 1e-5f);
}

// One corner of the surface, in clip space. Named, because five of these is
// otherwise five copies of one line number, and printed with both numbers
// because which side of the boundary it came out on is the whole finding.
//
// THE EXPLANATION IS PRINTED ONCE. One constant decides every corner, so a real
// failure is all of them at once and nine lines said five times would bury the
// numbers that differ.
static void strictly_inside_the_near_plane(const char *corner,
					   voe_math_float4 clip)
{
	static bool explained;

	if (!(clip.z < clip.w)) {
		fprintf(stderr,
			"      the %s corner of a screen-filling element surface came out at z %.9g, w %.9g\n",
			corner, (double)clip.z, (double)clip.w);
		if (!explained) {
			explained = true;
			fprintf(stderr,
				"      z must be STRICTLY less than w. At z == w every vertex of the surface\n"
				"      stands exactly on Vulkan's near clip boundary. The view volume is\n"
				"      0 <= z <= w inclusive, so a conformant driver keeps it — but a real\n"
				"      driver discarded the whole surface there and nothing mapped onto the\n"
				"      window reached the screen, while the same records drew perfectly\n"
				"      through the same pipeline on another driver. The z constant in\n"
				"      voe_render_element_transform belongs just inside the plane and not on\n"
				"      it; the comment at that constant says why 0.9999 and why not something\n"
				"      further back.\n");
		}
	}
	VOE_TEST_CHECK(clip.z < clip.w);
}

// The surface's corners are INSIDE the near clip boundary and not on it, which
// is the one claim on this path that a picture from this machine cannot make.
//
// IT IS ARITHMETIC AND THAT IS THE POINT. No graphics card, no headless device
// and no window: a matrix multiply and a comparison. The fault this pins was
// invisible to every test and every picture here, because the software
// rasteriser this repository checks on draws boundary geometry perfectly and a
// real driver did not — so nothing that renders can be the test, and the value
// has to be asserted where it is decided.
//
// STRICTLY LESS, NOT LESS-OR-EQUAL. Writing <= here would pass against the
// constant that caused bug 001 and would pin nothing at all.
//
// AND THE SCREEN-FILLING SURFACE STOPS SHORT OF THE NEAR CLIP BOUNDARY, WHICH IS
// THE ONE CLAIM NO PICTURE FROM THIS MACHINE COULD MAKE. Standing exactly on the
// boundary is valid by the specification and drew perfectly on the software
// rasteriser this repository checks on, and a real driver threw the whole
// surface away — so the assert is arithmetic on the matrix, z strictly less than
// w, and there is nothing to render. See bug 001 and ADR-0111.
static void the_surface_stops_short_of_the_near_clip_boundary(void)
{
	// A 240 by 135 mm surface, the shape a window's worth of millimetres
	// comes out at, and all four of its corners: they run through the same
	// row of the same matrix, so any of them landing on the boundary is the
	// fault, and asserting one of them would be asserting less than the
	// function does.
	voe_math_float2 size = { 240.0f, 135.0f };
	voe_math_float4x4 m = voe_render_element_transform(size);
	voe_math_float4 top_left = voe_math_float4x4_mul_float4(
		m, (voe_math_float4){ 0.0f, 0.0f, 0.0f, 1.0f });
	voe_math_float4 top_right = voe_math_float4x4_mul_float4(
		m, (voe_math_float4){ 240.0f, 0.0f, 0.0f, 1.0f });
	voe_math_float4 bottom_left = voe_math_float4x4_mul_float4(
		m, (voe_math_float4){ 0.0f, 135.0f, 0.0f, 1.0f });
	voe_math_float4 bottom_right = voe_math_float4x4_mul_float4(
		m, (voe_math_float4){ 240.0f, 135.0f, 0.0f, 1.0f });

	strictly_inside_the_near_plane("top-left", top_left);
	strictly_inside_the_near_plane("top-right", top_right);
	strictly_inside_the_near_plane("bottom-left", bottom_left);
	strictly_inside_the_near_plane("bottom-right", bottom_right);

	// And still inside the volume at the other end: retreating from the near
	// boundary onto the far one would be the same bug facing the other way.
	VOE_TEST_CHECK(top_left.z > 0.0f);

	// Every surface goes through this row whatever its size, so a second
	// size with nothing in common with the first says the constant is a
	// constant and not something that happens to work at one shape.
	m = voe_render_element_transform((voe_math_float2){ 37.0f, 1000.0f });
	strictly_inside_the_near_plane(
		"far", voe_math_float4x4_mul_float4(
			       m, (voe_math_float4){ 37.0f, 1000.0f, 0.0f,
						     1.0f }));
}

// Eighty bytes, said out loud here and not only in render/src/descriptors.c.
// The static asserts over there are the safety net and they fire at build time;
// this is the claim stated where a person reading the tests can see it, because
// the size is the promise the glyph kind was written to keep — every draw ever
// built against this record changes with it.
//
// It runs whether or not there is a graphics card, because it is arithmetic.
static void the_record_is_still_eighty_bytes(void)
{
	VOE_TEST_CHECK_INT((int)sizeof(voe_render_element), 80);
	// And the two words the glyph kind took are where the shader has them,
	// with two still spare after the first of them.
	VOE_TEST_CHECK_INT((int)offsetof(voe_render_element, kind), 48);
	VOE_TEST_CHECK_INT((int)offsetof(voe_render_element, sheet_texture), 52);
	VOE_TEST_CHECK_INT((int)offsetof(voe_render_element, sheet), 64);
}

int main(void)
{
	the_transform_puts_the_origin_at_the_top_left();
	the_surface_stops_short_of_the_near_clip_boundary();
	the_surface_matrix_is_millimetres_into_metres();
	the_surface_size_divides_both_axes_by_one_scale();
	ui_scale_at_two_halves_the_millimetres();
	the_record_is_still_eighty_bytes();
	return voe_test_result();
}
