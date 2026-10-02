// The point shadow maps (ADR-0325 points 1 and 2): that a device made with
// point_shadow_size has them and one without does not, that either still
// draws, and that the point-shadow pass draws a caster once or not at all.
//
// WITH 64, READY. The device says voe_render_point_shadows_ready, then draws two
// frames, so each frame slot's set is bound once, of a lit cube in one window
// pass: each frame ends true with one draw command.
//
// THE PASS, ON THE READY DEVICE. One frame with two lights slotted 1 and 2 opens
// the point-shadow pass: a cube 2 m from the first adds one draw command, the
// same cube 100 m off adds none. A camera pass with the same lights follows, and
// then a third pass, past `passes`, is refused.
//
// WITH 0, NOT READY, AND THE SAME TWO FRAMES. One texel a side stays for the
// binding, so the frames are identical.
//
// A card without shaderOutputLayer makes the first claim false by design; the
// device says so on stderr. With the validation layer present, a wrong layout or
// descriptor for binding 9 is a message there too.
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
#define POINT_SHADOW_SIDE 64

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 36,
	.geometries = 1,
	.objects = 2,
	.shadings = 1,
	.passes = 2,
};

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// A cube half a unit a side, inside an identity camera's clip volume.
static const voe_render_vertex VERTICES[8] = {
	{ { -0.25f, -0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.25f, -0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.25f, 0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { -0.25f, 0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { -0.25f, -0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.25f, -0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.25f, 0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { -0.25f, 0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
};

static const uint32_t INDICES[36] = {
	0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
	3, 6, 2, 3, 7, 6, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5,
};

// Two frames of the cube in one lit window pass, each ending with one draw.
static void draw_two_frames(voe_render_device *device, voe_render_geometry cube,
			    voe_render_shading grey)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = {
		.view = { .view = voe_math_float4x4_identity(),
			  .projection = voe_math_float4x4_identity() },
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .intensity = 1.0f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
	};
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = grey.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};

	for (int frame = 0; frame < 2; frame++) {
		bool drawing = false;

		VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
		VOE_TEST_CHECK(drawing);
		if (!drawing)
			return;
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object));
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);
	}
}

// One frame: the point-shadow pass with a near and a far cube, a camera pass,
// and a third pass refused.
static void draw_point_shadow_pass(voe_render_device *device,
				   voe_render_geometry cube,
				   voe_render_shading grey)
{
	voe_platform_size size = { SIDE, SIDE };
	const voe_render_point_light lamps[2] = {
		{ .position = { 2.0f, 0.0f, 0.5f }, .range = 5.0f,
		  .colour = { 1.0f, 1.0f, 1.0f }, .falloff = 1.0f,
		  .shadow = 1, .shadow_strength = 1.0f },
		{ .position = { -2.0f, 0.0f, 0.5f }, .range = 5.0f,
		  .colour = { 1.0f, 1.0f, 1.0f }, .falloff = 1.0f,
		  .shadow = 2, .shadow_strength = 1.0f },
	};
	const voe_render_point_lights points = { lamps, 2 };
	voe_render_pass_camera camera = {
		.view = { .view = voe_math_float4x4_identity(),
			  .projection = voe_math_float4x4_identity() },
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .intensity = 1.0f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
		.points = points,
	};
	voe_render_object near = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = grey.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	voe_render_object far = near;
	bool drawing = false;
	uint32_t before;

	far.world = voe_math_float4x4_from_translation(
		(voe_math_float3){ 100.0f, 0.0f, 0.0f });
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_point_shadow_pass_begin(device, &points));
	before = voe_render_frame_draw_count(device);
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, near));
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), before + 1);
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, far));
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), before + 1);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, near));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(!voe_render_point_shadow_pass_begin(device, &points));
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

// A device of `side`, checked ready or not and drawn; false when this machine
// has no Vulkan to test.
static bool check_device(voe_base_arena *arena, uint32_t side, bool ready)
{
	voe_render_capacities room = CAPACITIES;
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_geometry cube;
	voe_render_shading grey;
	voe_render_device *device;

	room.point_shadow_size = side;
	device = voe_render_device_new_headless(arena, size, room, &error);
	if (device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
			       error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		return false;
	}
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return true;

	VOE_TEST_CHECK(voe_render_point_shadows_ready(device) == ready);
	VOE_TEST_CHECK(voe_render_shading_create(device, GREY, &grey, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(device, VERTICES, 8, INDICES,
						  36, &cube, &error));
	draw_two_frames(device, cube, grey);
	if (ready)
		draw_point_shadow_pass(device, cube, grey);
	voe_render_device_destroy(device);
	return true;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	if (check_device(arena, POINT_SHADOW_SIDE, true))
		check_device(arena, 0, false);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
