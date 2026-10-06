// THE DEVICE KEEPS ANOTHER THREAD'S UPLOADS OUT OF AN OPEN FRAME (ADR-0370
// point 5). On the headless device with room for one element: a C11 thread
// creates and destroys 100 one-pixel textures and 100 one-triangle geometries
// while the main thread runs 100 element-only frames, each one pass with no
// camera, one element and one draw, as tests/elements.c draws one. Every call
// answers true, the worker is joined and the device destroyed.
//
// WHAT FAILS IT is a guard that lets an upload rewrite a set or use the queue
// while a frame is recorded or submitted: in a debug build the validation layer
// reports it and the device's destroy asserts on the count, and a guard that
// deadlocks never finishes. The worker counts its own refusals and the main
// thread checks the count after the join, because the test counter is not
// atomic.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO, as the others do.
#include <render/device.h>

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdint.h>
#include <stdio.h>
#include <threads.h>

#define ROUNDS 100
#define SIDE 16

static const voe_render_capacities CAPACITIES = {
	.vertices = 3,
	.indices = 3,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.elements = 1,
	.passes = 1,
};

static const uint8_t TEXEL[4] = { 255, 255, 255, 255 };

struct worker {
	voe_render_device *device;
	int refused;
};

// One texture and one geometry made and given back, ROUNDS times.
static int upload_and_free(void *context)
{
	struct worker *worker = context;
	const voe_render_vertex vertices[3] = {
		{ { -1.0f, -1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 1.0f, -1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 0.0f, 1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.5f, 1.0f } },
	};
	const uint32_t indices[3] = { 0, 1, 2 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_texture texture;
	voe_render_geometry geometry;

	for (int i = 0; i < ROUNDS; i++) {
		if (!voe_render_texture_create(worker->device,
					       VOE_RENDER_TEXTURE_COLOUR,
					       VOE_RENDER_SAMPLING_SHARP, 1, 1,
					       TEXEL, &texture, &error) ||
		    !voe_render_texture_destroy(worker->device, texture))
			worker->refused++;
		if (!voe_render_geometry_create(worker->device, vertices, 3,
						indices, 3, &geometry, &error) ||
		    !voe_render_geometry_destroy(worker->device, geometry))
			worker->refused++;
	}
	return 0;
}

// One frame of one pass with no camera, one element and its one draw.
static void element_frame(voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_element element = {
		.bounds = { 0.0f, 0.0f, SIDE, SIDE },
		.clip = { 0.0f, 0.0f, SIDE, SIDE },
		.colour = { 1.0f, 0.0f, 0.0f, 1.0f },
		.kind = VOE_RENDER_ELEMENT_SOLID,
	};
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     NULL));
	VOE_TEST_CHECK(voe_render_frame_submit_element(device, element));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(
		device,
		voe_render_element_transform((voe_math_float2){ SIDE, SIDE }),
		0, 1));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	struct worker worker = { 0 };
	thrd_t thread;
	int answer = -1;

	worker.device = voe_render_device_new_headless(
		arena, (voe_platform_size){ SIDE, SIDE }, CAPACITIES, &error);
	if (worker.device == NULL) {
		voe_base_arena_destroy(arena);
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			return voe_test_result();
		}
		VOE_TEST_CHECK(false);
		return voe_test_result();
	}

	if (thrd_create(&thread, upload_and_free, &worker) == thrd_success) {
		for (int i = 0; i < ROUNDS; i++)
			element_frame(worker.device);
		VOE_TEST_CHECK_INT(thrd_join(thread, &answer), thrd_success);
		VOE_TEST_CHECK_INT(answer, 0);
		VOE_TEST_CHECK_INT(worker.refused, 0);
	} else {
		VOE_TEST_CHECK(false);
	}

	voe_render_device_destroy(worker.device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
