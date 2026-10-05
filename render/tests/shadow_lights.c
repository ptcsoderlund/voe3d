// The shadow array growing to hold several directional lights (ADR-0357): that
// voe_render_shadow_lights_ready answers with what the array holds, that a frame
// wanting more grows it at the top of the next, that the grown layers are drawn
// into and read, and that it is capped and never shrinks. One device, asked in
// order; each claim builds on the one before.
//
// ONE LIGHT AT FIRST. ready(1) on a fresh device with shadow_size is 1; on a
// device without shadow_size it is 0.
//
// TWO ARE WANTED, ONE IS READY THAT FRAME, TWO THE NEXT. ready(2) inside an open
// frame answers 1; the next frame's begin grows the array and ready(2) is 2.
//
// THE SECOND LIGHT'S LAYERS DRAW AND READ. A shadow pass onto layer 7, the
// second light's last cascade, opens and ends, and a window pass whose camera
// reads shadow.slot 1 draws in the same frame, which ends true.
//
// NINE ARE CAPPED AT FOUR. ready(9), then a frame, then ready is 4.
//
// NOUGHT SHRINKS NOTHING. ready(0) is 4, the device still holds four lights and
// slot 0's sixteenth layer view is still made — read through device_internal.h.
//
// With the validation layer present, a stale view in binding 5 or a wrong layout
// on a grown layer is a message on stderr; the frames ending true is the rest.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include <render/device.h>

#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

#define SIDE 16
#define SHADOW_SIDE 64

static const voe_render_capacities CAPACITIES = {
	.vertices = 3,
	.indices = 3,
	.geometries = 1,
	.objects = 4,
	.shadings = 1,
	.passes = 2,
	.shadow_size = SHADOW_SIDE,
};

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

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

static bool open_frame(voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	return drawing;
}

// One frame of nothing but its begin and end: what grows the array.
static void empty_frame(voe_render_device *device)
{
	if (open_frame(device))
		VOE_TEST_CHECK(voe_render_frame_end(device));
}

static bool none_without_shadow_size(voe_base_arena *arena)
{
	voe_render_capacities room = CAPACITIES;
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;

	room.shadow_size = 0;
	device = voe_render_device_new_headless(arena, size, room, &error);
	if (device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
			       error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		return false;
	}
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return true;
	VOE_TEST_CHECK_INT(voe_render_shadow_lights_ready(device, 2), 0);
	empty_frame(device);
	VOE_TEST_CHECK_INT(voe_render_shadow_lights_ready(device, 2), 0);
	voe_render_device_destroy(device);
	return true;
}

static void the_array_grows(voe_base_arena *arena)
{
	static const voe_render_vertex vertices[3] = {
		{ { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.0f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	};
	static const uint32_t indices[3] = { 0, 1, 2 };
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_geometry triangle;
	voe_render_shading grey;
	voe_render_view light = identity_view();
	voe_render_pass_camera camera = {
		.view = identity_view(),
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .intensity = 1.0f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
		.shadow = { .splits = { 100.0f }, .count = 1, .slot = 1 },
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);

	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return;
	VOE_TEST_CHECK(voe_render_shading_create(device, GREY, &grey, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(device, vertices, 3, indices,
						  3, &triangle, &error));
	camera.shadow.cascades[0] = voe_math_float4x4_identity();

	VOE_TEST_CHECK_INT(voe_render_shadow_lights_ready(device, 1), 1);

	if (open_frame(device)) {
		VOE_TEST_CHECK_INT(voe_render_shadow_lights_ready(device, 2), 1);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}

	if (open_frame(device)) {
		VOE_TEST_CHECK_INT(voe_render_shadow_lights_ready(device, 2), 2);
		VOE_TEST_CHECK(voe_render_shadow_pass_begin(device, 7, &light));
		VOE_TEST_CHECK(voe_render_frame_draw(device, triangle,
						     object(grey)));
		voe_render_pass_end(device);
		VOE_TEST_CHECK(!voe_render_pass_is_open(device));
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		VOE_TEST_CHECK(voe_render_frame_draw(device, triangle,
						     object(grey)));
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 2);
	}

	VOE_TEST_CHECK_INT(voe_render_shadow_lights_ready(device, 9), 2);
	empty_frame(device);
	VOE_TEST_CHECK_INT(voe_render_shadow_lights_ready(device, 9), 4);

	VOE_TEST_CHECK_INT(voe_render_shadow_lights_ready(device, 0), 4);
	empty_frame(device);
	VOE_TEST_CHECK_INT(device->shadow_lights, 4);
	VOE_TEST_CHECK(device->frames[0].shadow.layers[15] != VK_NULL_HANDLE);
	VOE_TEST_CHECK(device->frames[1].shadow.layers[15] != VK_NULL_HANDLE);
	voe_render_device_destroy(device);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);

	if (none_without_shadow_size(arena))
		the_array_grows(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
