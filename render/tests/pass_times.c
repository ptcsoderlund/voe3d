// The frame breakdown (ADR-0367): every pass timed and named by render, read
// back by voe_render_frame_pass_times. Headless, `shadow_size` set, `passes` 8.
//
// A SHADOW PASS THEN A WINDOW PASS ARE LISTED IN THAT ORDER. Frames of a shadow
// pass onto cascade 0 and a camera pass onto the window, run past
// VOE_RENDER_FRAMES_IN_FLIGHT so the newest measured frame is one of them: the
// breakdown is `shadow light 0 cascade 0` then `view window`, each above nought,
// their sum not above voe_render_frame_gpu_time's.
//
// A CAPACITY OF 1 WRITES ONE, the first pass.
//
// A PASS NOT RUN IS ABSENT. Frames after with no shadow pass, again past the
// lag, list `view window` alone.
//
// A SPAN IS LISTED AFTER ITS PASS (ADR-0396 point 6). Frames whose window pass
// wraps its draw in a span `terrain` list `view window` then
// `view window: terrain`, the span above nought and not above the pass.
//
// A CARD WHOSE GPU TIME NEVER ANSWERS writes no timestamps: the test says so and
// passes. A machine with no usable Vulkan skips and says so.
#include <render/device.h>

// For VOE_RENDER_FRAMES_IN_FLIGHT, the lag the breakdown is read at.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#define SIDE 64
#define SHADOW_SIDE 256
// Frames enough that the newest measured one is the last kind drawn.
#define FRAMES (VOE_RENDER_FRAMES_IN_FLIGHT + 2)
// The pass sum and the frame are each ticks times the period; their rounding
// may differ by far less than this.
#define ROUNDING 1e-12

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 36,
	.geometries = 1,
	.objects = 8,
	.shadings = 1,
	.passes = 8,
	.shadow_size = SHADOW_SIDE,
};

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

struct scene {
	voe_render_device *device;
	voe_render_geometry cube;
	voe_render_shading grey;
};

// A cube half a unit a side, inside an identity camera's clip volume.
static bool upload_cube(struct scene *scene)
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

	return voe_render_geometry_create(scene->device, vertices, 8, indices,
					  36, &scene->cube, &error);
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

// One frame: a shadow pass onto cascade 0 when `shadow`, then a camera pass onto
// the window, each drawing the cube; the window's draw inside a span `terrain`
// when `span`.
static void draw_frame(struct scene *scene, bool shadow, bool span)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_view light = identity_view();
	voe_render_pass_camera camera = {
		.view = identity_view(),
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .intensity = 1.0f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
	};
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	if (shadow) {
		VOE_TEST_CHECK(voe_render_shadow_pass_begin(scene->device, 0,
							    &light));
		VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->cube,
						     object(scene->grey)));
		voe_render_pass_end(scene->device);
	}
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device,
					     VOE_RENDER_TARGET_WINDOW, &camera));
	if (span)
		voe_render_frame_span_begin(scene->device, "terrain");
	VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->cube,
					     object(scene->grey)));
	if (span)
		voe_render_frame_span_end(scene->device);
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
}

static void shadow_then_window(struct scene *scene, double frame_seconds)
{
	voe_render_pass_time times[4];
	uint32_t count = voe_render_frame_pass_times(scene->device, times, 4);
	double sum = 0.0;

	VOE_TEST_CHECK_INT(count, 2);
	if (count != 2)
		return;
	printf("%s %.9f s, %s %.9f s, frame %.9f s\n", times[0].name,
	       times[0].seconds, times[1].name, times[1].seconds, frame_seconds);
	VOE_TEST_CHECK(strcmp(times[0].name, "shadow light 0 cascade 0") == 0);
	VOE_TEST_CHECK(strcmp(times[1].name, "view window") == 0);
	for (uint32_t i = 0; i < count; i++) {
		VOE_TEST_CHECK(times[i].seconds > 0.0);
		sum += times[i].seconds;
	}
	VOE_TEST_CHECK(sum <= frame_seconds + ROUNDING);

	count = voe_render_frame_pass_times(scene->device, times, 1);
	VOE_TEST_CHECK_INT(count, 1);
	VOE_TEST_CHECK(strcmp(times[0].name, "shadow light 0 cascade 0") == 0);
}

static void window_alone(struct scene *scene)
{
	voe_render_pass_time times[4];
	uint32_t count;

	for (int i = 0; i < FRAMES; i++)
		draw_frame(scene, false, false);
	count = voe_render_frame_pass_times(scene->device, times, 4);
	VOE_TEST_CHECK_INT(count, 1);
	if (count == 1)
		VOE_TEST_CHECK(strcmp(times[0].name, "view window") == 0);
}

static void span_after_pass(struct scene *scene)
{
	voe_render_pass_time times[4];
	uint32_t count;

	for (int i = 0; i < FRAMES; i++)
		draw_frame(scene, false, true);
	count = voe_render_frame_pass_times(scene->device, times, 4);
	VOE_TEST_CHECK_INT(count, 2);
	if (count != 2)
		return;
	printf("%s %.9f s, %s %.9f s\n", times[0].name, times[0].seconds,
	       times[1].name, times[1].seconds);
	VOE_TEST_CHECK(strcmp(times[0].name, "view window") == 0);
	VOE_TEST_CHECK(strcmp(times[1].name, "view window: terrain") == 0);
	VOE_TEST_CHECK(times[1].seconds > 0.0);
	VOE_TEST_CHECK(times[1].seconds <= times[0].seconds + ROUNDING);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };
	double frame_seconds = 0.0;

	scene.device = voe_render_device_new_headless(arena, size, CAPACITIES,
						      &error);
	if (scene.device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
				     error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	VOE_TEST_CHECK(scene.device != NULL);
	if (scene.device == NULL) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, GREY, &scene.grey,
						 &error));
	VOE_TEST_CHECK(upload_cube(&scene));

	for (int i = 0; i < FRAMES; i++)
		draw_frame(&scene, true, false);
	if (!voe_render_frame_gpu_time(scene.device, &frame_seconds)) {
		printf("skip: this card writes no timestamps, so there is no breakdown\n");
		VOE_TEST_CHECK_INT(voe_render_frame_pass_times(scene.device, NULL, 0),
				   0);
	} else {
		shadow_then_window(&scene, frame_seconds);
		window_alone(&scene);
		span_after_pass(&scene);
	}

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
