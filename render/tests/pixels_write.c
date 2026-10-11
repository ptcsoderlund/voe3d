// A FRAME WRITES PIXELS INTO AN RGBA8 TEXTURE (ADR-0402 point 4). An 8 × 8 black
// FIELD texture has its 4 × 4 corner written red inside a frame with
// voe_render_texture_write_pixels, taking 16 of the slot's 64 heights_texels; an
// 8 × 8 write after it is refused REFUSED and the frame still draws. The frame's
// one draw is an unlit quad that fills the picture wearing the red corner (uv
// 0..0.5) as its base colour, and the picture's centre reads red: the copy
// reached the image before the fragment stage read it.
//
// THE TEXTURE IS 8 × 8, NOT 4 × 4: an 8 × 8 write into a 4 × 4 texture is a
// rectangle past it, which asserts before the room is asked.
//
// The quad and camera are terrain_vertex.c's: a camera at the origin looking down
// −z, the quad at z −1 wide enough to cover the view. The read is
// voe_render_target_read, as there.
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
#define TEXTURE_SIDE 8
#define RED_SIDE 4
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
	.heights_texels = 64,
};

// Facing the eye, counter-clockwise seen from +z, past the view's edges.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -2.0f, 2.0f, -1.0f }, { 0, 0, 1 }, { 0, 0 } },
	{ { 2.0f, 2.0f, -1.0f }, { 0, 0, 1 }, { 0.5f, 0 } },
	{ { 2.0f, -2.0f, -1.0f }, { 0, 0, 1 }, { 0.5f, 0.5f } },
	{ { -2.0f, -2.0f, -1.0f }, { 0, 0, 1 }, { 0, 0.5f } },
};

static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

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

// The frame: the red write taken, the big one refused, the quad drawn, and the
// window's picture read into `arena`.
static voe_render_picture draw_written(voe_render_device *device,
				       voe_base_arena *arena,
				       voe_render_geometry quad,
				       voe_render_shading shading,
				       voe_render_texture texture)
{
	static uint8_t red[RED_SIDE * RED_SIDE * 4];
	static const uint8_t big[TEXTURE_SIDE * TEXTURE_SIDE * 4];
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
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	for (int i = 0; i < RED_SIDE * RED_SIDE; i++) {
		red[i * 4] = 255;
		red[i * 4 + 3] = 255;
	}

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	VOE_TEST_CHECK(voe_render_texture_write_pixels(device, texture, 0, 0,
						       RED_SIDE, RED_SIDE, red,
						       &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
	VOE_TEST_CHECK(!voe_render_texture_write_pixels(device, texture, 0, 0,
							TEXTURE_SIDE,
							TEXTURE_SIDE, big,
							&error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, quad, object));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	return picture;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	voe_render_geometry quad = { 0 };
	voe_render_shading shading = { 0 };
	voe_render_texture texture = { 0 };
	static const uint8_t black[TEXTURE_SIDE * TEXTURE_SIDE * 4];

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

	VOE_TEST_CHECK(voe_render_geometry_create(device, QUAD_VERTICES, 4,
						  QUAD_INDICES, 6, &quad, &error));
	VOE_TEST_CHECK(voe_render_texture_create(device, VOE_RENDER_TEXTURE_DATA,
						 VOE_RENDER_SAMPLING_FIELD,
						 TEXTURE_SIDE, TEXTURE_SIDE,
						 black, &texture, &error));
	voe_render_shading_values values = {
		.base_colour = { 1, 1, 1, 1 },
		.unlit = 1,
		.base_colour_texture = texture.index,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &shading,
						 &error));

	voe_render_picture picture =
		draw_written(device, arena, quad, shading, texture);
	if (picture.pixels != NULL) {
		const uint8_t *centre =
			picture.pixels +
			((size_t)(SIDE / 2) * picture.width + SIDE / 2) * 4;

		VOE_TEST_CHECK(abs(centre[0] - 255) <= TOLERANCE);
		VOE_TEST_CHECK(centre[1] <= TOLERANCE);
		VOE_TEST_CHECK(centre[2] <= TOLERANCE);
	} else {
		VOE_TEST_CHECK(false);
	}

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
