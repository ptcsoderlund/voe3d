// The sun's bounce map (ADR-0308 point 1): that a frame can draw its casters into
// the bounce map after the cascades, that the pass counts like any other, and
// that the camera pass after it still draws. Three claims; the last reads a pixel.
//
// OUTSIDE A FRAME THE CALL IS FALSE, and leaves no pass open.
//
// FOUR CASCADES, A BOUNCE PASS AND A WINDOW PASS ARE ONE FRAME. With room for
// exactly those six passes a cube draws in each; the window pass closes the open
// bounce pass, one more pass is refused, and the frame ends true with six draws.
// Twice, so both frame slots' maps are drawn and handed back to compute.
//
// A CAMERA PASS AFTER IT STILL DRAWS. A red cube drawn into a bounce pass left
// open, then into the window unshaded, reads red at the middle of the picture.
// A bounce pass left open at the frame's end is closed by it: the frame still
// ends true.
//
// With the validation layer present, a wrong layout, barrier or attachment in
// any of these is a message on stderr; the frames ending true is the rest.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include <render/device.h>

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

#define SIDE 16
#define SHADOW_SIDE 64
#define PASSES (1 + VOE_RENDER_SHADOW_CASCADES + 1)

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 36,
	.geometries = 1,
	.objects = PASSES,
	.shadings = 1,
	.passes = PASSES,
	.shadow_size = SHADOW_SIDE,
};

static const voe_render_shading_values RED = {
	.base_colour = { 1.0f, 0.0f, 0.0f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

static const voe_render_light SUN = {
	.direction = { 0.0f, -1.0f, 0.0f },
	.intensity = 1.0f,
	.colour = { 1.0f, 1.0f, 1.0f },
};

// A cube half a unit a side, well inside an identity camera's clip volume.
static bool upload_cube(voe_render_device *device, voe_render_geometry *out)
{
	static const voe_render_vertex vertices[8] = {
		{ { -0.25f, -0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, -0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, 0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { -0.25f, 0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { -0.25f, -0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, -0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, 0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { -0.25f, 0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	};
	static const uint32_t indices[36] = {
		0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
		3, 6, 2, 3, 7, 6, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5,
	};
	voe_base_error error = VOE_BASE_OK;

	return voe_render_geometry_create(device, vertices, 8, indices, 36, out,
					  &error);
}

static voe_render_view identity_view(void)
{
	return (voe_render_view){
		.view = voe_math_float4x4_identity(),
		.projection = voe_math_float4x4_identity(),
	};
}

static voe_render_object object(voe_render_shading shading)
{
	return (voe_render_object){
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// A device with the red record and the cube, or NULL — and *skip set when the
// reason is that this machine has no Vulkan to test.
static voe_render_device *open_device(voe_base_arena *arena,
				      voe_render_geometry *cube,
				      voe_render_shading *red, bool *skip)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);

	*skip = device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
				   error == VOE_BASE_ERROR_UNSUPPORTED);
	if (*skip)
		printf("skip: %s\n", voe_base_error_string(error));
	if (device == NULL)
		return NULL;

	VOE_TEST_CHECK(voe_render_shading_create(device, RED, red, &error));
	VOE_TEST_CHECK(upload_cube(device, cube));
	return device;
}

static bool open_frame(voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	return drawing;
}

// A window pass through an identity camera, unshaded, with the cube drawn.
static void window_pass(voe_render_device *device, voe_render_geometry cube,
			voe_render_shading red)
{
	voe_render_pass_camera camera = { .view = identity_view(), .light = SUN };

	camera.light.unshaded = 1;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object(red)));
	voe_render_pass_end(device);
}

static void outside_a_frame_is_false(voe_render_device *device)
{
	voe_render_view light = identity_view();

	VOE_TEST_CHECK(!voe_render_bounce_pass_begin(device, &light, &SUN));
	VOE_TEST_CHECK(!voe_render_pass_is_open(device));
}

static void cascades_bounce_then_the_window(voe_render_device *device,
					    voe_render_geometry cube,
					    voe_render_shading red)
{
	voe_render_view light = identity_view();

	for (int frame = 0; frame < 2 && open_frame(device); frame++) {
		for (uint32_t i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
			VOE_TEST_CHECK(voe_render_shadow_pass_begin(device, i,
								    &light));
			VOE_TEST_CHECK(voe_render_frame_draw(device, cube,
							     object(red)));
			voe_render_pass_end(device);
		}
		VOE_TEST_CHECK(voe_render_bounce_pass_begin(device, &light, &SUN));
		VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object(red)));
		// No _pass_end: the window pass closes the bounce pass.
		window_pass(device, cube, red);
		VOE_TEST_CHECK(!voe_render_bounce_pass_begin(device, &light, &SUN));
		VOE_TEST_CHECK(!voe_render_pass_is_open(device));
		VOE_TEST_CHECK(voe_render_frame_end(device));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), PASSES);
	}
}

static void a_camera_pass_after_it_reads_red(voe_render_device *device,
					     voe_render_geometry cube,
					     voe_render_shading red,
					     voe_base_arena *arena)
{
	voe_render_view light = identity_view();
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	for (int frame = 0; frame < 2 && open_frame(device); frame++) {
		VOE_TEST_CHECK(voe_render_bounce_pass_begin(device, &light, &SUN));
		VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object(red)));
		window_pass(device, cube, red);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	if (picture.pixels != NULL) {
		const uint8_t *middle =
			&picture.pixels[((size_t)(picture.height / 2) *
						 picture.width +
					 picture.width / 2) *
					4];

		printf("middle: %d %d %d\n", middle[0], middle[1], middle[2]);
		VOE_TEST_CHECK(middle[0] > 200);
		VOE_TEST_CHECK(middle[1] < 50);
		VOE_TEST_CHECK(middle[2] < 50);
	}

	// A bounce pass left open is closed by the frame's end.
	if (open_frame(device)) {
		VOE_TEST_CHECK(voe_render_bounce_pass_begin(device, &light, &SUN));
		VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object(red)));
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	voe_render_geometry cube;
	voe_render_shading red;
	bool skip;
	voe_render_device *device = open_device(arena, &cube, &red, &skip);

	if (!skip) {
		VOE_TEST_CHECK(device != NULL);
		if (device != NULL) {
			outside_a_frame_is_false(device);
			cascades_bounce_then_the_window(device, cube, red);
			a_camera_pass_after_it_reads_red(device, cube, red,
							 arena);
			voe_render_device_destroy(device);
		}
	}

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
