// The Best Practices gate (ADR-0358, 0367 point 6), on the headless device.
//
// IN A DEBUG BUILD THE CHECKS ARE ON. The device opened says checks_on, read
// through ../src/device_internal.h as card.c reads startup.h; a debug build
// that could not load the validation layer fails here, the one place it does.
//
// A FRAME OF TODAY'S PASSES CLOSES CLEAN. A shadow pass onto cascade 0, then a
// camera pass onto the window that draws a cube, copies its depth and draws an
// element. voe_render_device_destroy is the gate: a validation error or a
// warning not on the allowlist asserts there and this test fails.
//
// A RELEASE BUILD has no layer and no gate: the test says so and passes. A
// machine with no usable Vulkan skips and says so.
#include <render/device.h>

#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

#ifdef NDEBUG

int main(void)
{
	printf("skip: a release build runs no validation layer, so there is no gate\n");
	return voe_test_result();
}

#else

#define SIDE 64

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 36,
	.geometries = 1,
	.objects = 8,
	.shadings = 1,
	.elements = 1,
	.passes = 8,
	.shadow_size = 256,
};

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// A cube half a unit a side, inside an identity camera's clip volume.
static bool upload_cube(voe_render_device *device, voe_render_geometry *cube)
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

	return voe_render_geometry_create(device, vertices, 8, indices, 36, cube,
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

static void draw_frame(voe_render_device *device, voe_render_geometry cube,
		       voe_render_shading grey)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_view light = identity_view();
	voe_render_pass_camera camera = {
		.view = identity_view(),
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .intensity = 1.0f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
	};
	voe_render_element element = {
		.bounds = { 4.0f, 4.0f, 8.0f, 8.0f },
		.clip = { 0.0f, 0.0f, SIDE, SIDE },
		.colour = { 1.0f, 0.0f, 0.0f, 1.0f },
	};
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_shadow_pass_begin(device, 0, &light));
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object(grey)));
	voe_render_pass_end(device);

	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object(grey)));
	VOE_TEST_CHECK(voe_render_frame_copy_depth(device));
	VOE_TEST_CHECK(voe_render_frame_submit_element(device, element));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(
		device, voe_render_element_transform((voe_math_float2){ SIDE, SIDE }),
		0, 1));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

int main(void)
{
	voe_base_arena *arena;
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;
	voe_render_geometry cube = { 0 };
	voe_render_shading grey = { 0 };

	arena = voe_base_arena_new(64 * 1024);
	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
			       error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	VOE_TEST_CHECK(device->checks_on);
	VOE_TEST_CHECK(!device->checks_missing);
	VOE_TEST_CHECK(voe_render_shading_create(device, GREY, &grey, &error));
	VOE_TEST_CHECK(upload_cube(device, &cube));
	draw_frame(device, cube, grey);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}

#endif
