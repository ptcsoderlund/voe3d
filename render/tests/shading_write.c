// A SHADING RECORD REWRITTEN INSIDE A FRAME DRAWS ITS NEW VALUES THAT FRAME
// (ADR-0399 point 4). One quad fills the middle of a 64 × 64 picture, seen by a
// camera at the origin looking down −z, in a pass whose light is `unshaded` so
// the centre reads the base colour alone:
//
// 1. drawn with its record as created, red: the centre reads red;
// 2. the next frame writes the record blue before its pass: the centre reads
//    blue, so the write lands before that frame's own passes;
// 3. three frames in a row write red, green, then blue, with no read and no
//    wait between them, so earlier frames are still in flight when the later
//    writes are recorded: the centre reads blue, the last.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO. The skip is a pass and it
// prints its reason.
#include <render/device.h>

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SIDE 64

// How far a channel may be from the expected byte: a driver's rounding on the
// way into an sRGB target.
#define TOLERANCE 3

#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.passes = 1,
};

// Two metres ahead, facing the eye, counter-clockwise seen from +z.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -1.0f, -1.0f, -2.0f }, { 0, 0, 1 }, { 0, 0 } },
	{ { 1.0f, -1.0f, -2.0f }, { 0, 0, 1 }, { 0, 0 } },
	{ { 1.0f, 1.0f, -2.0f }, { 0, 0, 1 }, { 0, 0 } },
	{ { -1.0f, 1.0f, -2.0f }, { 0, 0, 1 }, { 0, 0 } },
};

static const uint32_t QUAD_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

static const voe_math_float4 RED = { 1.0f, 0.0f, 0.0f, 1.0f };
static const voe_math_float4 GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
static const voe_math_float4 BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };

// At the origin looking down −z, reverse-Z.
static voe_render_view the_camera(void)
{
	voe_render_view view = { .view = voe_math_float4x4_identity() };
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

static voe_render_shading_values coloured(voe_math_float4 colour)
{
	voe_render_shading_values values = {
		.base_colour = colour,
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};

	return values;
}

// One frame drawing the quad, its record written `colour` first when `colour`
// is not NULL.
static void draw_frame(voe_render_device *device, voe_render_geometry quad,
		       voe_render_shading shading,
		       const voe_math_float4 *colour)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = { .unshaded = 1 },
	};
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1, 1, 1, 1 },
	};
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	if (colour != NULL)
		voe_render_shading_write(device, shading, coloured(*colour));
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, quad, object));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

// The centre of the frame that ended last is `colour`.
static void check_centre(voe_render_device *device, voe_base_arena *arena,
			 voe_math_float4 colour)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const uint8_t *pixel;

	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	if (picture.pixels == NULL)
		return;
	pixel = picture.pixels +
		((size_t)(SIDE / 2) * picture.width + SIDE / 2) * 4;
	VOE_TEST_CHECK(abs(pixel[0] - (int)lroundf(colour.x * 255.0f)) <=
		       TOLERANCE);
	VOE_TEST_CHECK(abs(pixel[1] - (int)lroundf(colour.y * 255.0f)) <=
		       TOLERANCE);
	VOE_TEST_CHECK(abs(pixel[2] - (int)lroundf(colour.z * 255.0f)) <=
		       TOLERANCE);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	voe_render_geometry quad = { 0 };
	voe_render_shading shading = { 0 };

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	VOE_TEST_CHECK(voe_render_geometry_create(device, QUAD_VERTICES, 4,
						  QUAD_INDICES, 6, &quad, &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, coloured(RED), &shading,
						 &error));

	draw_frame(device, quad, shading, NULL);
	check_centre(device, arena, RED);

	draw_frame(device, quad, shading, &BLUE);
	check_centre(device, arena, BLUE);

	draw_frame(device, quad, shading, &RED);
	draw_frame(device, quad, shading, &GREEN);
	draw_frame(device, quad, shading, &BLUE);
	check_centre(device, arena, BLUE);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
