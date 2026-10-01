// Every target's bounce grid (ADR-0308 point 3): that the window and each caller
// target own one, built with them, kept through a resize and freed with them, and
// that drawing still works around them, and the update into them. Three claims.
//
// TWO TARGETS, MADE, RESIZED AND FREED TWICE. A device with `targets` 2 makes
// both, each with its grid built; resizes both, and the next frame applies the
// resize with the same grid handles; draws a red cube into each, unshaded, and
// reads red at the middle of each at its new size; then is destroyed, which
// frees every grid. Twice, on two devices one after the other.
//
// THE WINDOW DRAWS A RED CUBE TOO, with its own grid built by the device.
//
// THE UPDATE (ADR-0308 point 4), two frames on the two live targets: false with
// no bounce pass yet and inside a camera pass; after one, true on a target,
// false a second time on it, true on the other and on the window, false for an
// id whose generation names no live target (render has no target destroy, so a
// stale id is what a destroyed one would be). A camera pass then names the
// updated grid in its block, and the one before the update named none.
//
// With the validation layer present, a wrong layout, clear or descriptor write in
// any of these is a message on stderr; the frames ending true is the rest.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

#define SIDE 16
#define WIDE 24
#define TARGETS 2
#define PASSES (TARGETS + 4)

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 36,
	.geometries = 1,
	.objects = PASSES,
	.shadings = 1,
	.passes = PASSES,
	.targets = TARGETS,
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

static const struct voe_render_bounce_update UPDATE = {
	.cell = { -16, -16, -16 },
	.corner = { -32.0f, -32.0f, -32.0f },
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

// A pass onto `target` through an identity camera, unshaded, with the cube.
static void red_pass(voe_render_device *device, voe_render_target target,
		     voe_render_geometry cube, voe_render_shading red)
{
	voe_render_pass_camera camera = {
		.view = { .view = voe_math_float4x4_identity(),
			  .projection = voe_math_float4x4_identity() },
		.light = { .direction = { 0.0f, -1.0f, 0.0f }, .unshaded = 1 },
	};
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = red.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};

	VOE_TEST_CHECK(voe_render_pass_begin(device, target, &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object));
	voe_render_pass_end(device);
}

// The middle pixel of `target`'s picture is red, and the picture is `width` wide.
static void reads_red(voe_render_device *device, voe_render_target target,
		      uint32_t width, voe_base_arena *arena)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const uint8_t *middle;

	VOE_TEST_CHECK(voe_render_target_read(device, target, arena, &picture,
					      &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	if (picture.pixels == NULL)
		return;
	VOE_TEST_CHECK_INT(picture.width, width);
	middle = &picture.pixels[((size_t)(picture.height / 2) * picture.width +
				  picture.width / 2) *
				 4];
	printf("middle: %d %d %d\n", middle[0], middle[1], middle[2]);
	VOE_TEST_CHECK(middle[0] > 200);
	VOE_TEST_CHECK(middle[1] < 50);
	VOE_TEST_CHECK(middle[2] < 50);
}

// Two frames, so both frame slots draw: a red pass onto each target, when
// `targets` is not NULL, then onto the window.
static void draw_frames(voe_render_device *device,
			const voe_render_target *targets,
			voe_render_geometry cube, voe_render_shading red)
{
	voe_platform_size size = { SIDE, SIDE };

	for (int frame = 0; frame < 2; frame++) {
		bool drawing = false;

		VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
		VOE_TEST_CHECK(drawing);
		if (!drawing)
			return;
		for (uint32_t i = 0; targets != NULL && i < TARGETS; i++)
			red_pass(device, targets[i], cube, red);
		red_pass(device, VOE_RENDER_TARGET_WINDOW, cube, red);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
}

// The grid the block of the pass just begun names.
static uint32_t named_grid(const voe_render_device *device)
{
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	const struct voe_render_frame_block *block =
		(const void *)((const char *)frame->uniforms_mapped +
			       (device->pass_count - 1) * device->pass_stride);

	return block->bounce.grid;
}

// One frame of the update's claims on the two live `targets`.
static void one_update_frame(voe_render_device *device,
			     const voe_render_target *targets,
			     voe_render_geometry cube, voe_render_shading red)
{
	voe_render_pass_camera camera = {
		.view = { .view = voe_math_float4x4_identity(),
			  .projection = voe_math_float4x4_identity() },
		.light = SUN,
	};
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = red.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	voe_render_target stale = targets[1];
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	stale.generation++;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(!voe_render_bounce_update(device, targets[0], &UPDATE));
	VOE_TEST_CHECK(voe_render_pass_begin(device, targets[0], &camera));
	VOE_TEST_CHECK_INT(named_grid(device), VOE_RENDER_NO_BOUNCE);
	VOE_TEST_CHECK(!voe_render_bounce_update(device, targets[0], &UPDATE));
	voe_render_pass_end(device);

	VOE_TEST_CHECK(voe_render_bounce_pass_begin(device, &camera.view, &SUN));
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object));
	VOE_TEST_CHECK(voe_render_bounce_update(device, targets[0], &UPDATE));
	VOE_TEST_CHECK(!voe_render_bounce_update(device, targets[0], &UPDATE));
	VOE_TEST_CHECK(voe_render_bounce_update(device, targets[1], &UPDATE));
	VOE_TEST_CHECK(!voe_render_bounce_update(device, stale, &UPDATE));
	VOE_TEST_CHECK(voe_render_bounce_update(device, VOE_RENDER_TARGET_WINDOW,
						&UPDATE));

	VOE_TEST_CHECK(voe_render_pass_begin(device, targets[0], &camera));
	VOE_TEST_CHECK_INT(named_grid(device), device->targets[0].grid.descriptor);
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

static void two_targets_keep_their_grids(voe_render_device *device,
					 voe_render_geometry cube,
					 voe_render_shading red,
					 voe_base_arena *arena)
{
	voe_render_target targets[TARGETS];
	VkImage kept[TARGETS];
	voe_base_error error = VOE_BASE_OK;

	for (uint32_t i = 0; i < TARGETS; i++) {
		voe_render_texture texture;

		VOE_TEST_CHECK(voe_render_target_create(device, SIDE, SIDE,
							&targets[i], &texture,
							&error));
		kept[i] = device->targets[i].grid.sh[2].image;
		VOE_TEST_CHECK(kept[i] != VK_NULL_HANDLE);
		VOE_TEST_CHECK_INT(device->targets[i].grid.descriptor,
				   3 * (i + 1));
		voe_render_target_resize(device, targets[i], WIDE, SIDE);
	}
	draw_frames(device, targets, cube, red);
	for (uint32_t i = 0; i < TARGETS; i++) {
		VOE_TEST_CHECK(device->targets[i].grid.sh[2].image == kept[i]);
		reads_red(device, targets[i], WIDE, arena);
	}
	for (int frame = 0; frame < 2; frame++)
		one_update_frame(device, targets, cube, red);
}

static void the_window_draws_red(voe_render_device *device,
				 voe_render_geometry cube,
				 voe_render_shading red, voe_base_arena *arena)
{
	VOE_TEST_CHECK(device->window_grid.sh[0].image != VK_NULL_HANDLE);
	VOE_TEST_CHECK_INT(device->window_grid.descriptor, 0);
	draw_frames(device, NULL, cube, red);
	reads_red(device, VOE_RENDER_TARGET_WINDOW, SIDE, arena);
}

// One device's life: open, both cases, destroy. False when it skipped.
static bool one_device(voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_geometry cube;
	voe_render_shading red;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);

	if (device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
			       error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		return false;
	}
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return false;

	VOE_TEST_CHECK(voe_render_shading_create(device, RED, &red, &error));
	VOE_TEST_CHECK(upload_cube(device, &cube));
	the_window_draws_red(device, cube, red, arena);
	two_targets_keep_their_grids(device, cube, red, arena);
	voe_render_device_destroy(device);
	return true;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);

	for (int round = 0; round < 2 && one_device(arena); round++)
		;

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
