// The shadow pass (ADR-0258): that a frame can draw depth into the sun's four
// cascades and then its picture, and that it counts like any other pass. Three
// claims, none of which reads a pixel — nothing samples the maps until card 04.
//
// A DEVICE WITHOUT shadow_size DRAWS AS BEFORE. Nought is the default every
// existing caller has, and it still opens, draws a cube into the window and ends.
//
// WITH IT, FOUR SHADOW PASSES AND A WINDOW PASS ARE ONE FRAME. Each cascade gets
// a pass with the cube drawn into it, the window gets one more, the frame ends
// true and holds five draw commands — the shadow draws counted like any other.
//
// A SHADOW PASS PAST `passes` IS REFUSED AND THE FRAME STILL ENDS. Room for two:
// the third shadow pass returns false, leaves no pass open, and _end is true.
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

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 36,
	.geometries = 1,
	.objects = 5,
	.shadings = 1,
	.passes = 5,
};

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// A cube half a unit a side about the origin, well inside an identity camera's
// clip volume. Its normals do not matter: nothing here is looked at.
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

// A device of `room`, with the grey record and the cube, or NULL — and *skip set
// when the reason is that this machine has no Vulkan to test.
static voe_render_device *open_device(voe_base_arena *arena,
				      voe_render_capacities room,
				      voe_render_geometry *cube,
				      voe_render_shading *grey, bool *skip)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, room, &error);

	*skip = device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
				   error == VOE_BASE_ERROR_UNSUPPORTED);
	if (*skip)
		printf("skip: %s\n", voe_base_error_string(error));
	if (device == NULL)
		return NULL;

	VOE_TEST_CHECK(voe_render_shading_create(device, GREY, grey, &error));
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

static void window_pass(voe_render_device *device, voe_render_geometry cube,
			voe_render_shading grey)
{
	voe_render_pass_camera camera = {
		.view = identity_view(),
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .intensity = 1.0f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
	};

	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object(grey)));
	voe_render_pass_end(device);
}

static bool no_shadow_draws_as_before(voe_base_arena *arena)
{
	voe_render_geometry cube;
	voe_render_shading grey;
	bool skip;
	voe_render_device *device =
		open_device(arena, CAPACITIES, &cube, &grey, &skip);

	if (skip)
		return false;
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return true;

	if (open_frame(device)) {
		window_pass(device, cube, grey);
		VOE_TEST_CHECK(voe_render_frame_end(device));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);
	}
	voe_render_device_destroy(device);
	return true;
}

static void four_cascades_then_the_window(voe_base_arena *arena)
{
	voe_render_capacities room = CAPACITIES;
	voe_render_geometry cube;
	voe_render_shading grey;
	voe_render_view light = identity_view();
	bool skip;
	voe_render_device *device;

	room.shadow_size = SHADOW_SIDE;
	device = open_device(arena, room, &cube, &grey, &skip);
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return;

	// Twice, so each frame slot's maps are drawn and handed back once.
	for (int frame = 0; frame < 2 && open_frame(device); frame++) {
		for (uint32_t i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
			VOE_TEST_CHECK(voe_render_shadow_pass_begin(device, i,
								    &light));
			VOE_TEST_CHECK(voe_render_frame_draw(device, cube,
							     object(grey)));
			voe_render_pass_end(device);
		}
		window_pass(device, cube, grey);
		VOE_TEST_CHECK(voe_render_frame_end(device));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 5);
	}
	voe_render_device_destroy(device);
}

static void a_shadow_pass_past_passes_is_refused(voe_base_arena *arena)
{
	voe_render_capacities room = CAPACITIES;
	voe_render_geometry cube;
	voe_render_shading grey;
	voe_render_view light = identity_view();
	bool skip;
	voe_render_device *device;

	room.shadow_size = SHADOW_SIDE;
	room.passes = 2;
	device = open_device(arena, room, &cube, &grey, &skip);
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return;

	if (open_frame(device)) {
		for (uint32_t i = 0; i < 2; i++) {
			VOE_TEST_CHECK(voe_render_shadow_pass_begin(device, i,
								    &light));
			voe_render_pass_end(device);
		}
		VOE_TEST_CHECK(!voe_render_shadow_pass_begin(device, 2, &light));
		VOE_TEST_CHECK(!voe_render_pass_is_open(device));
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
	voe_render_device_destroy(device);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	if (no_shadow_draws_as_before(arena)) {
		four_cascades_then_the_window(arena);
		a_shadow_pass_past_passes_is_refused(arena);
	}

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
