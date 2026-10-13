// A PAINTED LANDSCAPE DRAWS ITS LAYERS (ADR-0402 point 5). A flat 2-cell
// landscape, 200 m a side, heights all nought, is seen straight down through an
// orthographic camera 200 m wide, so it fills the picture and landscape x runs
// with the picture's columns. Its own record is unlit white; its layers 0 and 1
// are unlit records red and green with no maps, so both heights are one and the
// paint alone decides. The claims:
//
// A. paint 3 × 3, the left column all layer 0, the middle half and half, the
//    right all layer 1: every pixel of the picture's left quarter is red and of
//    its right quarter green, the bilinear weight there 0.75 or more and the
//    other layer's s more than 0.2 below;
// B. the same record with `paint_a` nought: every pixel the object's white;
// C. paint all layer 0, layer 1 green at weight nought everywhere: no pixel
//    green, every pixel red.
//
// THE GRID IS WOUND FACING UP, seen from above; the solid pipeline culls the
// other side. `paint_b` is nought throughout, layers 4–7 nought.
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

#define NEAR_PLANE 0.1f
#define FAR_PLANE 1000.0f

static const voe_render_capacities CAPACITIES = {
	.vertices = 9,
	.indices = 24,
	.geometries = 1,
	.objects = 1,
	.shadings = 3,
	.passes = 1,
};

// Vertex c + 3r lies at x = c / 2, z = r / 2.
static const voe_render_vertex GRID_VERTICES[9] = {
	{ { 0.0f, 0.0f, 0.0f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { 0.5f, 0.0f, 0.0f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { 1.0f, 0.0f, 0.0f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { 0.0f, 0.0f, 0.5f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { 0.5f, 0.0f, 0.5f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { 1.0f, 0.0f, 0.5f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { 0.0f, 0.0f, 1.0f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { 0.5f, 0.0f, 1.0f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { 1.0f, 0.0f, 1.0f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
};

// Each cell (corner, +x+z, +x) and (corner, +z, +x+z): counter-clockwise seen
// from above.
static const uint32_t GRID_INDICES[24] = {
	0, 4, 1, 0, 3, 4,	1, 5, 2, 1, 4, 5,
	3, 7, 4, 3, 6, 7,	4, 8, 5, 4, 7, 8,
};

// One weight texel a layer: layer 0 alone, half and half, layer 1 alone.
#define LAYER_0 255, 0, 0, 0
#define HALF 128, 127, 0, 0
#define LAYER_1 0, 255, 0, 0

static const uint8_t SPLIT_PAINT[36] = {
	LAYER_0, HALF, LAYER_1, LAYER_0, HALF, LAYER_1, LAYER_0, HALF, LAYER_1,
};

static const uint8_t GROUND_PAINT[36] = {
	LAYER_0, LAYER_0, LAYER_0, LAYER_0, LAYER_0,
	LAYER_0, LAYER_0, LAYER_0, LAYER_0,
};

typedef enum { WHITE, RED, GREEN } colour_name;

static const voe_math_float4 COLOURS[3] = {
	{ 1, 1, 1, 1 },
	{ 1, 0, 0, 1 },
	{ 0, 1, 0, 1 },
};

// Looking straight down −y, landscape +z down the picture; orthographic over
// 200 m, reverse-Z.
static voe_render_view the_camera(void)
{
	voe_render_view view = { 0 };
	float span = FAR_PLANE - NEAR_PLANE;

	view.view.m[0][0] = 1.0f;
	view.view.m[1][2] = -1.0f;
	view.view.m[2][1] = 1.0f;
	view.view.m[3][3] = 1.0f;
	view.projection.m[0][0] = 2.0f / 200.0f;
	view.projection.m[1][1] = 2.0f / 200.0f;
	view.projection.m[2][2] = 1.0f / span;
	view.projection.m[2][3] = FAR_PLANE / span;
	view.projection.m[3][3] = 1.0f;
	return view;
}

// One frame drawing `object`, then the window's picture read into `arena`.
static voe_render_picture draw_case(voe_render_device *device,
				    voe_base_arena *arena,
				    voe_render_geometry grid,
				    voe_render_object object)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = { .view = the_camera() };
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, grid, object));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	return picture;
}

// Whether the pixel at (row, column) is `name` within TOLERANCE; all three
// colours are 0 or 1, the same byte in sRGB as in linear.
static bool is_colour(voe_render_picture picture, int row, int column,
		      colour_name name)
{
	const uint8_t *pixel =
		picture.pixels + ((size_t)row * picture.width + (size_t)column) * 4;
	const float want[3] = { COLOURS[name].x, COLOURS[name].y,
				COLOURS[name].z };

	for (int channel = 0; channel < 3; channel++)
		if (abs(pixel[channel] - (int)lroundf(want[channel] * 255.0f)) >
		    TOLERANCE)
			return false;
	return true;
}

// How many pixels of columns `first` up to `last` are not `name`.
static int count_not(voe_render_picture picture, int first, int last,
		     colour_name name)
{
	int count = 0;

	for (int row = 0; row < SIDE; row++)
		for (int column = first; column < last; column++)
			count += !is_colour(picture, row, column, name);
	return count;
}

// How many pixels of the whole picture are `name`.
static int count_is(voe_render_picture picture, colour_name name)
{
	return SIDE * SIDE - count_not(picture, 0, SIDE, name);
}

static void check_pictures(voe_render_picture split, voe_render_picture bare,
			   voe_render_picture ground)
{
	VOE_TEST_CHECK_INT(count_not(split, 0, SIDE / 4, RED), 0);
	VOE_TEST_CHECK_INT(count_not(split, SIDE - SIDE / 4, SIDE, GREEN), 0);
	VOE_TEST_CHECK_INT(count_not(bare, 0, SIDE, WHITE), 0);
	VOE_TEST_CHECK_INT(count_is(ground, GREEN), 0);
	VOE_TEST_CHECK_INT(count_not(ground, 0, SIDE, RED), 0);
}

static bool make_shading(voe_render_device *device, colour_name name,
			 voe_render_shading *out, voe_base_error *error)
{
	voe_render_shading_values values = {
		.base_colour = COLOURS[name],
		.roughness = 1.0f,
		.unlit = 1,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};

	return voe_render_shading_create(device, values, out, error);
}

static bool make_paint(voe_render_device *device, const uint8_t *rgba,
		       voe_render_texture *out, voe_base_error *error)
{
	return voe_render_texture_create(device, VOE_RENDER_TEXTURE_DATA,
					 VOE_RENDER_SAMPLING_FIELD, 3, 3, rgba,
					 out, error);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	voe_render_geometry grid = { 0 };
	voe_render_shading white = { 0 };
	voe_render_shading red = { 0 };
	voe_render_shading green = { 0 };
	voe_render_texture heights = { 0 };
	voe_render_texture split_paint = { 0 };
	voe_render_texture ground_paint = { 0 };
	const float zeros[9] = { 0 };

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
	VOE_TEST_CHECK(make_shading(device, WHITE, &white, &error));
	VOE_TEST_CHECK(make_shading(device, RED, &red, &error));
	VOE_TEST_CHECK(make_shading(device, GREEN, &green, &error));
	VOE_TEST_CHECK(voe_render_texture_create_heights(device, 3, 3, zeros,
							 &heights, &error));
	VOE_TEST_CHECK(make_paint(device, SPLIT_PAINT, &split_paint, &error));
	VOE_TEST_CHECK(make_paint(device, GROUND_PAINT, &ground_paint, &error));

	// The node box 0..1 m of a 200 m landscape centred under the eye, 10 m
	// below it.
	voe_render_object split = {
		.world = { .m = { { 200, 0, 0, -100 },
				  { 0, 1, 0, -10 },
				  { 0, 0, 200, -100 },
				  { 0, 0, 0, 1 } } },
		.normal = voe_math_float4x4_identity(),
		.shading = white.index,
		.heights = heights.index + 1,
		.paint = { split_paint.index + 1, 0 },
		.colour = { 1, 1, 1, 1 },
		.terrain = { -100, -100, 200, 200 },
		.morph = { 1e5f, 2e5f, 0, 1 },
		.layers = { red.index, green.index },
	};
	voe_render_object bare = split;
	voe_render_object ground = split;

	bare.paint[0] = 0;
	ground.paint[0] = ground_paint.index + 1;

	voe_render_picture a = draw_case(device, arena, grid, split);
	voe_render_picture b = draw_case(device, arena, grid, bare);
	voe_render_picture c = draw_case(device, arena, grid, ground);

	if (a.pixels != NULL && b.pixels != NULL && c.pixels != NULL)
		check_pictures(a, b, c);
	else
		VOE_TEST_CHECK(false);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
