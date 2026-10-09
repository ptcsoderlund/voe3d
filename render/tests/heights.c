// A landscape's heights texture (ADR-0396 points 3 and 5): made at 2049 × 2049,
// the side 2048 cells need, then written a rectangle at a time inside frames
// from each frame slot's heights_texels.
//
// THE BUDGET IS PER FRAME AND REFUSED WHOLE. HEIGHTS_TEXELS is 20: a 4 × 4 write
// takes 16 of it, a 3 × 3 write after it is refused with REFUSED and the frame
// still draws a pass and ends true. The next frames, round both slots, start
// with nothing spent and take all 20 in one 5 × 4 write — a reset that forgot
// the count would refuse it.
//
// It includes render's internal header by relative path, as tests/transient.c
// does, to read the slot's spent count. A MACHINE WITH NO USABLE VULKAN SKIPS
// AND SAYS SO.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <stdlib.h>

#define SIDE 2049
#define HEIGHTS_TEXELS 20

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.passes = 1,
	.heights_texels = HEIGHTS_TEXELS,
};

// One frame: begin, the frame slot's budget nought spent, one write of all of
// it at the far corner, a pass with no camera after it, and the end.
static void frame_with_full_write(voe_render_device *device,
				  voe_render_texture heights,
				  const float *values)
{
	voe_platform_size size = { 16, 16 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	VOE_TEST_CHECK_INT(voe_render_frame_current(device)->heights_used, 0);
	VOE_TEST_CHECK(voe_render_texture_write_heights(device, heights, 2044,
							2045, 5, 4, values,
							&error));
	VOE_TEST_CHECK_INT(voe_render_frame_current(device)->heights_used,
			   HEIGHTS_TEXELS);
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     NULL));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

static void writes_in_frames(voe_render_device *device,
			     voe_render_texture heights)
{
	static const float VALUES[HEIGHTS_TEXELS] = {
		1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f,
		11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f,
	};
	voe_platform_size size = { 16, 16 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	VOE_TEST_CHECK(voe_render_texture_write_heights(device, heights, 0, 0, 4,
							4, VALUES, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
	VOE_TEST_CHECK(!voe_render_texture_write_heights(device, heights, 10, 10,
							 3, 3, VALUES, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);
	VOE_TEST_CHECK_INT(voe_render_frame_current(device)->heights_used, 16);
	VOE_TEST_CHECK(device->recording);
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     NULL));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));

	// Round both slots and back to the first: each budget whole again.
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT + 1; i++)
		frame_with_full_write(device, heights, VALUES);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_platform_size size = { 16, 16 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;
	voe_render_texture heights = { 0 };
	float *ground;

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

	// A gentle slope, metres as given.
	ground = malloc((size_t)SIDE * SIDE * sizeof(float));
	VOE_TEST_CHECK(ground != NULL);
	if (ground != NULL) {
		for (uint32_t i = 0; i < (uint32_t)SIDE * SIDE; i++)
			ground[i] = (float)(i % SIDE) * 0.01f;
		VOE_TEST_CHECK(voe_render_texture_create_heights(device, SIDE,
								 SIDE, ground,
								 &heights,
								 &error));
		free(ground);
	}
	VOE_TEST_CHECK(heights.index != VOE_RENDER_NO_TEXTURE);
	VOE_TEST_CHECK(!voe_render_texture_create_heights(device, 4097, 2,
							  (const float[8]){ 0 },
							  &(voe_render_texture){ 0 },
							  &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);

	if (heights.index != VOE_RENDER_NO_TEXTURE) {
		writes_in_frames(device, heights);
		VOE_TEST_CHECK(voe_render_texture_destroy(device, heights));
	}

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
