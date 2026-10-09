// A TERRAIN RECORD DRAWS THE SHARED GRID AT ITS HEIGHTS (ADR-0396 point 3). The
// claim is three pictures of one 2 × 2 quad grid, x and z in 0..1 at y 0, drawn
// by a camera at the origin looking level down −z:
//
// A. as terrain over a 3 × 3 heights texture, all 2 m, written inside the first
//    frame with _write_heights: the node is the whole 200 m landscape, its box
//    0..4 m, and `world` puts landscape y 1 at the eye, so the ground is a plane
//    1 m above it. Row 0's centre is the ground's colour;
// B. the same grid as a plain mesh scaled to 200 m and lifted to y 1: what a 2 m
//    plane draws. Every pixel of A's upper rows matches it;
// C. A's record with `heights` nought: the grid drawn as plain geometry, at
//    world y −1, below the eye. No pixel above the middle row is ground.
//
// THE GRID IS WOUND FACING DOWN. The solid pipeline culls back faces, and a
// plane above the eye is seen from below; wound up, A and B would both be
// culled and agree about nothing. C, seen from above, is culled for the same
// reason, which only makes its claim stricter.
//
// The pass's light is `unshaded`, so every surface reads its base colour and the
// normal the terrain computes does not move a pixel; the morph range is far
// past the grid, so nothing slides. Both are other claims.
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

// Rows A and B are compared over: the plane's far edge, 100 m off, lies a
// fraction of a pixel above the middle row, so the last two rows above it are
// left out.
#define COMPARED_ROWS (SIDE / 2 - 2)

#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 1000.0f

static const voe_render_capacities CAPACITIES = {
	.vertices = 9,
	.indices = 24,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.passes = 1,
	.heights_texels = 9,
};

// Vertex c + 3r lies at x = c / 2, z = r / 2.
static const voe_render_vertex GRID_VERTICES[9] = {
	{ { 0.0f, 0.0f, 0.0f }, { 0, 1, 0 }, { 0, 0 } },
	{ { 0.5f, 0.0f, 0.0f }, { 0, 1, 0 }, { 0, 0 } },
	{ { 1.0f, 0.0f, 0.0f }, { 0, 1, 0 }, { 0, 0 } },
	{ { 0.0f, 0.0f, 0.5f }, { 0, 1, 0 }, { 0, 0 } },
	{ { 0.5f, 0.0f, 0.5f }, { 0, 1, 0 }, { 0, 0 } },
	{ { 1.0f, 0.0f, 0.5f }, { 0, 1, 0 }, { 0, 0 } },
	{ { 0.0f, 0.0f, 1.0f }, { 0, 1, 0 }, { 0, 0 } },
	{ { 0.5f, 0.0f, 1.0f }, { 0, 1, 0 }, { 0, 0 } },
	{ { 1.0f, 0.0f, 1.0f }, { 0, 1, 0 }, { 0, 0 } },
};

// Each cell (corner, +x, +x+z) and (corner, +x+z, +z): counter-clockwise seen
// from below.
static const uint32_t GRID_INDICES[24] = {
	0, 1, 4, 0, 4, 3,	1, 2, 5, 1, 5, 4,
	3, 4, 7, 3, 7, 6,	4, 5, 8, 4, 8, 7,
};

static const voe_math_float4 BASE = { 0.2f, 0.8f, 0.4f, 1.0f };

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

// One frame drawing `object`, writing `heights` first when it is not NULL, then
// the window's picture read into `arena`.
static voe_render_picture draw_case(voe_render_device *device,
				    voe_base_arena *arena,
				    voe_render_geometry grid,
				    voe_render_object object,
				    voe_render_texture texture,
				    const float *heights)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = { .unshaded = 1 },
	};
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	if (heights != NULL)
		VOE_TEST_CHECK(voe_render_texture_write_heights(
			device, texture, 0, 0, 3, 3, heights, &error));
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, grid, object));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	return picture;
}

// A linear value as the byte an sRGB target stores for it.
static int srgb_byte(float linear)
{
	float encoded = linear <= 0.0031308f
				? linear * 12.92f
				: 1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;

	return (int)lroundf(encoded * 255.0f);
}

static const uint8_t *pixel_at(voe_render_picture picture, int row, int column)
{
	return picture.pixels + ((size_t)row * picture.width + (size_t)column) * 4;
}

static bool is_ground(const uint8_t *pixel)
{
	return abs(pixel[0] - srgb_byte(BASE.x)) <= TOLERANCE &&
	       abs(pixel[1] - srgb_byte(BASE.y)) <= TOLERANCE &&
	       abs(pixel[2] - srgb_byte(BASE.z)) <= TOLERANCE;
}

static bool same_pixel(const uint8_t *a, const uint8_t *b)
{
	for (int channel = 0; channel < 4; channel++)
		if (abs(a[channel] - b[channel]) > TOLERANCE)
			return false;
	return true;
}

static void check_pictures(voe_render_picture terrain, voe_render_picture plane,
			   voe_render_picture flat)
{
	int different = 0;
	int flat_ground = 0;

	VOE_TEST_CHECK(is_ground(pixel_at(terrain, 0, SIDE / 2)));
	for (int row = 0; row < COMPARED_ROWS; row++)
		for (int column = 0; column < SIDE; column++)
			different += !same_pixel(pixel_at(terrain, row, column),
						 pixel_at(plane, row, column));
	for (int row = 0; row < SIDE / 2; row++)
		for (int column = 0; column < SIDE; column++)
			flat_ground += is_ground(pixel_at(flat, row, column));
	VOE_TEST_CHECK_INT(different, 0);
	VOE_TEST_CHECK_INT(flat_ground, 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	voe_render_geometry grid = { 0 };
	voe_render_shading shading = { 0 };
	voe_render_texture heights = { 0 };
	voe_render_shading_values values = {
		.base_colour = BASE,
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	const float zeros[9] = { 0 };
	const float two[9] = { 2, 2, 2, 2, 2, 2, 2, 2, 2 };

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			voe_base_arena_destroy(arena);
			return voe_test_result();
		}
		VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	VOE_TEST_CHECK(voe_render_geometry_create(device, GRID_VERTICES, 9,
						  GRID_INDICES, 24, &grid, &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &shading,
						 &error));
	VOE_TEST_CHECK(voe_render_texture_create_heights(device, 3, 3, zeros,
							 &heights, &error));

	// The node box 0..4 m of a 200 m landscape centred on the origin, its
	// landscape y 1 at the eye.
	voe_render_object terrain = {
		.world = { .m = { { 200, 0, 0, -100 },
				  { 0, 4, 0, -1 },
				  { 0, 0, 200, -100 },
				  { 0, 0, 0, 1 } } },
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.heights = heights.index + 1,
		.colour = { 1, 1, 1, 1 },
		.terrain = { -100, -100, 200, 200 },
		.morph = { 1e5f, 2e5f, 0, 4 },
	};
	voe_render_object plane = terrain;
	voe_render_object flat = terrain;

	plane.heights = 0;
	plane.world.m[1][1] = 1;
	plane.world.m[1][3] = 1;
	flat.heights = 0;

	voe_render_picture a = draw_case(device, arena, grid, terrain, heights, two);
	voe_render_picture b = draw_case(device, arena, grid, plane, heights, NULL);
	voe_render_picture c = draw_case(device, arena, grid, flat, heights, NULL);

	if (a.pixels != NULL && b.pixels != NULL && c.pixels != NULL)
		check_pictures(a, b, c);
	else
		VOE_TEST_CHECK(false);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
