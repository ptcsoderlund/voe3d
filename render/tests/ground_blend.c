// A GROUND RECORD BLENDS INTO THE PAINTED GROUND (ADR-0481 point 1). A flat
// 200 m landscape, heights all nought, painted all layer 0, an unlit red record;
// only its textures and records are made, its nodes never drawn. An unlit white
// square fills the picture, seen straight down through an orthographic camera
// 200 m wide; its record is a ground record of that landscape, reach 1 m, the
// landscape's ground 10 m below the eye. The claims:
//
// A. the square 0.1 m above the ground draws red, t ≈ 0.97: every pixel's red
//    over 230, its green and blue under 60 — the 0.028 left of white is about
//    47 once the sRGB target encodes it;
// B. at 2 m, past the reach, every pixel white;
// C. at 0.1 m with `ground` nought, every pixel white;
// D. at 0.5 m every channel of every pixel between A's and B's, green and blue
//    clear of both.
//
// THE SQUARE IS WOUND FACING UP, seen from above; the solid pipeline culls the
// other side.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO. The skip is a pass and it
// prints its reason.
#include <render/device.h>

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

#define SIDE 64

// How far a channel may be from the expected byte: a driver's rounding on the
// way into an sRGB target.
#define TOLERANCE 3

#define NEAR_PLANE 0.1f
#define FAR_PLANE 1000.0f

// The ground's height under the eye, in camera-relative metres.
#define GROUND_Y -10.0f

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 2,
	.passes = 1,
};

// A 2 × 2 square about the origin at y nought.
static const voe_render_vertex SQUARE_VERTICES[4] = {
	{ { -1.0f, 0.0f, -1.0f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { 1.0f, 0.0f, -1.0f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { -1.0f, 0.0f, 1.0f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { 1.0f, 0.0f, 1.0f }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
};

// (corner, +x+z, +x) and (corner, +z, +x+z): counter-clockwise from above.
static const uint32_t SQUARE_INDICES[6] = { 0, 3, 1, 0, 2, 3 };

#define LAYER_0 255, 0, 0, 0

static const uint8_t GROUND_PAINT[36] = {
	LAYER_0, LAYER_0, LAYER_0, LAYER_0, LAYER_0,
	LAYER_0, LAYER_0, LAYER_0, LAYER_0,
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

// One frame drawing `object` over `square`, then the window's picture read
// into `arena`.
static voe_render_picture draw_case(voe_render_device *device,
				    voe_base_arena *arena,
				    voe_render_geometry square,
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
	VOE_TEST_CHECK(voe_render_frame_draw(device, square, object));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	return picture;
}

// The `channel` byte of pixel `i`.
static int channel_of(voe_render_picture picture, int i, int channel)
{
	return picture.pixels[(size_t)i * 4 + (size_t)channel];
}

// How many pixels have red over `red_above` and green and blue under
// `rest_below`.
static int count_red(voe_render_picture picture, int red_above, int rest_below)
{
	int count = 0;

	for (int i = 0; i < SIDE * SIDE; i++)
		count += channel_of(picture, i, 0) > red_above &&
			 channel_of(picture, i, 1) < rest_below &&
			 channel_of(picture, i, 2) < rest_below;
	return count;
}

// How many pixels have every channel within TOLERANCE of 255.
static int count_white(voe_render_picture picture)
{
	int count = 0;

	for (int i = 0; i < SIDE * SIDE; i++)
		count += channel_of(picture, i, 0) >= 255 - TOLERANCE &&
			 channel_of(picture, i, 1) >= 255 - TOLERANCE &&
			 channel_of(picture, i, 2) >= 255 - TOLERANCE;
	return count;
}

// How many pixels of `mid` have every channel between `low`'s and `high`'s
// within TOLERANCE, and green and blue more than TOLERANCE clear of both.
static int count_between(voe_render_picture low, voe_render_picture mid,
			 voe_render_picture high)
{
	int count = 0;

	for (int i = 0; i < SIDE * SIDE; i++) {
		bool between = true;

		for (int channel = 0; channel < 3; channel++) {
			int a = channel_of(low, i, channel);
			int b = channel_of(high, i, channel);
			int m = channel_of(mid, i, channel);
			int lo = a < b ? a : b;
			int hi = a < b ? b : a;

			between = between && m >= lo - TOLERANCE &&
				  m <= hi + TOLERANCE;
			if (channel > 0)
				between = between && m > lo + TOLERANCE &&
					  m < hi - TOLERANCE;
		}
		count += between;
	}
	return count;
}

static bool make_shading(voe_render_device *device, voe_math_float4 colour,
			 voe_render_shading *out, voe_base_error *error)
{
	voe_render_shading_values values = {
		.base_colour = colour,
		.roughness = 1.0f,
		.unlit = 1,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};

	return voe_render_shading_create(device, values, out, error);
}

// `object` with its square `above` metres over the ground.
static voe_render_object raised(voe_render_object object, float above)
{
	object.world.m[1][3] = GROUND_Y + above;
	return object;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	voe_render_geometry square = { 0 };
	voe_render_shading white = { 0 };
	voe_render_shading red = { 0 };
	voe_render_texture heights = { 0 };
	voe_render_texture paint = { 0 };
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

	VOE_TEST_CHECK(voe_render_geometry_create(device, SQUARE_VERTICES, 4,
						  SQUARE_INDICES, 6, &square,
						  &error));
	VOE_TEST_CHECK(make_shading(device, (voe_math_float4){ 1, 1, 1, 1 },
				    &white, &error));
	VOE_TEST_CHECK(make_shading(device, (voe_math_float4){ 1, 0, 0, 1 },
				    &red, &error));
	VOE_TEST_CHECK(voe_render_texture_create_heights(device, 3, 3, zeros,
							 &heights, &error));
	VOE_TEST_CHECK(voe_render_texture_create(device, VOE_RENDER_TEXTURE_DATA,
						 VOE_RENDER_SAMPLING_FIELD, 3, 3,
						 GROUND_PAINT, &paint, &error));

	// The square 200 m a side; the landscape's own metres the camera-
	// relative ones raised by 10 m, so its ground is GROUND_Y.
	voe_render_object model = {
		.world = { .m = { { 100, 0, 0, 0 },
				  { 0, 1, 0, 0 },
				  { 0, 0, 100, 0 },
				  { 0, 0, 0, 1 } } },
		.normal = voe_math_float4x4_identity(),
		.shading = white.index,
		.paint = { paint.index + 1, 0 },
		.colour = { 1, 1, 1, 1 },
		.terrain = { 0, 0, 0, 200 },
		.layers = { red.index },
		.ground = heights.index + 1,
		.ground_reach = 1.0f,
		.ground_rows = { { 1, 0, 0, 0 },
				 { 0, 1, 0, -GROUND_Y },
				 { 0, 0, 1, 0 } },
	};
	voe_render_object bare = raised(model, 0.1f);

	bare.ground = 0;

	voe_render_picture a = draw_case(device, arena, square, raised(model, 0.1f));
	voe_render_picture b = draw_case(device, arena, square, raised(model, 2.0f));
	voe_render_picture c = draw_case(device, arena, square, bare);
	voe_render_picture d = draw_case(device, arena, square, raised(model, 0.5f));

	if (a.pixels != NULL && b.pixels != NULL && c.pixels != NULL &&
	    d.pixels != NULL) {
		VOE_TEST_CHECK_INT(count_red(a, 230, 60), SIDE * SIDE);
		VOE_TEST_CHECK_INT(count_white(b), SIDE * SIDE);
		VOE_TEST_CHECK_INT(count_white(c), SIDE * SIDE);
		VOE_TEST_CHECK_INT(count_between(a, d, b), SIDE * SIDE);
	} else {
		VOE_TEST_CHECK(false);
	}

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
